# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Resolve and cache credentials for IronSide Artifactory downloads.

Credentials are checked in this order: explicit token, environment variable,
OS keyring, netrc, then browser sign-in. Browser sign-in tokens are replaced
with shorter-lived tokens before storage.
"""

from __future__ import annotations

import json
import logging as log
import os
import socket
import sys
import time
import uuid
import webbrowser
from contextlib import closing
from urllib.parse import quote, urlparse

import requests

from .python_version import require_python

require_python()

# Repository tokens use a separate service from firmware signing keys.
SERVICE = "ironside-artifactory"
TOKEN_ENV = "IRONSIDE_ARTIFACTORY_TOKEN"
BACKEND_ENV = "PYTHON_KEYRING_BACKEND"

# This name is displayed on the Artifactory sign-in page.
CLIENT_NAME = "IronSide downloader"

LOGIN_TIMEOUT = 300.0
POLL_INTERVAL = 3.0
CACHE_TTL = 365 * 24 * 3600
EXPIRY_SKEW = 300  # Do not start a download with a nearly expired token.


class AuthError(Exception):
    """Artifactory authentication failed."""


def platform_root(base_url: str) -> str:
    """Return the JFrog platform root for an Artifactory repository URL."""
    root, sep, _ = base_url.rstrip("/").partition("/artifactory/")
    if not sep:
        raise AuthError(
            f"not an Artifactory repository URL: {base_url}. "
            "Expected <host>/artifactory/<repo>/<path>"
        )
    return root


def bearer(token: str):
    """Return a requests auth callable for a bearer token.

    Using session.auth prevents requests from replacing the bearer token with
    credentials from netrc while preserving proxy settings.
    """

    def apply(request):
        request.headers["Authorization"] = f"Bearer {token}"
        return request

    return apply


# --------------------------------------------------------------------------- #
# The OS keyring
# --------------------------------------------------------------------------- #

_kr = None


def _keyring():
    """Import keyring on first use and select one backend.

    Selecting the first backend avoids password prompts from other backends
    when the token is not stored. Raises KeyringUnavailable when the backend
    could only be read after a password prompt this session cannot show.
    """
    global _kr
    if _kr is None:
        # Hide keyring's debug messages for backends it does not select.
        log.getLogger("keyring").setLevel(log.WARNING)
        try:
            import keyring
        except ImportError as exc:
            raise KeyringUnavailable("the `keyring` package is not installed") from exc
        if not os.environ.get(BACKEND_ENV):
            try:
                backend = keyring.get_keyring()
            except Exception as exc:
                raise KeyringUnavailable(f"no usable backend ({type(exc).__name__})") from exc
            chained = getattr(backend, "backends", None)
            if chained:
                keyring.set_keyring(chained[0])
        reason = _secret_service_prompt_reason(keyring.get_keyring())
        if reason:
            raise KeyringUnavailable(reason)
        _kr = keyring
    return _kr


def _secret_service_prompt_reason(backend) -> str:
    """Why reading @backend needs a password prompt that cannot be shown, or "".

    Secret Service shows its unlock and create-collection prompts on the desktop
    session. Without a display, for example over SSH, the request waits for the
    prompt forever instead of failing.
    """
    if os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY"):
        return ""
    try:
        import secretstorage
        from keyring.backends.SecretService import Keyring as SecretServiceKeyring
        from secretstorage.collection import DEFAULT_COLLECTION
    except ImportError:
        return ""
    if not isinstance(backend, SecretServiceKeyring):
        return ""
    path = getattr(backend, "preferred_collection", DEFAULT_COLLECTION)
    try:
        with closing(secretstorage.dbus_init()) as bus:
            locked = secretstorage.Collection(bus, path).is_locked()
    except secretstorage.exceptions.ItemNotFoundException:
        return (
            "the Secret Service has no default collection, and creating one "
            "needs a password prompt this session cannot show"
        )
    except Exception as exc:
        return f"the Secret Service cannot be reached ({type(exc).__name__}: {exc})"
    if locked:
        return (
            "the Secret Service collection is locked, and unlocking it needs a "
            "password prompt this session cannot show"
        )
    return ""


class KeyringUnavailable(Exception):
    """No usable keyring is available."""


def _account(root: str) -> str:
    """Return the keyring account name for a JFrog platform root."""
    return urlparse(root).hostname or root


def _cache_entry(root: str):
    """Return the stored entry, or None if it cannot be read."""
    try:
        raw = _keyring().get_password(SERVICE, _account(root))
    except KeyringUnavailable as exc:
        log.info("not using a stored sign-in: %s", exc)
        return None
    except Exception as exc:
        log.debug("keyring read failed: %s", exc)
        return None
    if not raw:
        return None
    try:
        entry = json.loads(raw)
        entry["token"], entry["expires_at"] = entry["token"], float(entry["expires_at"])
    except (ValueError, KeyError, TypeError):
        log.debug("keyring entry for %s is not readable. Ignoring it", _account(root))
        return None
    return entry


def _cache_read(root: str):
    """Return a usable stored token, or None."""
    entry = _cache_entry(root)
    if entry is None:
        return None
    if time.time() + EXPIRY_SKEW >= entry["expires_at"]:
        log.debug("cached token for %s has expired", _account(root))
        return None
    return entry["token"]


def _cache_write(
    root: str, token: str, expires_in: float, username: str = "", token_id: str = ""
) -> None:
    """Store a token and the ID needed to revoke it."""
    entry = {
        "token": token,
        "expires_at": time.time() + expires_in,
        "username": username,
        "token_id": token_id,
    }
    try:
        _keyring().set_password(SERVICE, _account(root), json.dumps(entry))
    except Exception as exc:
        log.warning(
            "could not store the token in the OS keyring (%s). This sign-in will not be remembered",
            exc,
        )
        return
    log.debug("stored a token for %s in the OS keyring", _account(root))


def forget(base_url: str) -> bool:
    """Delete and revoke the stored token. Return True if one existed."""
    root = platform_root(base_url)
    # Read the token ID before deleting the entry. Revocation is best effort.
    revoked = _withdraw(root)
    try:
        kr = _keyring()
        if kr.get_password(SERVICE, _account(root)) is None:
            return False
        kr.delete_password(SERVICE, _account(root))
        if revoked:
            log.debug("revoked the stored token as well as forgetting it")
    except KeyringUnavailable as exc:
        raise AuthError(f"no OS keyring on this host to forget from: {exc}") from exc
    except Exception as exc:
        raise AuthError(f"OS keyring: cannot delete {SERVICE}/{_account(root)}: {exc}") from exc
    return True


def _withdraw(root: str) -> bool:
    """Revoke a stored token when its token ID is available."""
    entry = _cache_entry(root)
    if not entry or not entry.get("token_id"):
        return False
    return _revoke(root, entry["token"], entry["token_id"])


# --------------------------------------------------------------------------- #
# Browser sign-in
# --------------------------------------------------------------------------- #


def _anonymous() -> requests.Session:
    """Return a session that omits netrc credentials but retains proxies."""
    session = requests.Session()
    session.auth = lambda request: request
    return session


def _revoke(root: str, token: str, token_id: str) -> bool:
    """Revoke a token. Return True if the server accepted the request."""
    try:
        resp = _anonymous().delete(
            f"{root}/access/api/v1/tokens/{token_id}",
            headers={"Authorization": f"Bearer {token}"},
            timeout=30,
        )
        return resp.ok
    except requests.RequestException as exc:
        log.debug("could not revoke %s: %s", token_id, exc)
        return False


def _bounded(root: str, issued: dict):
    """Replace the browser token with one that lasts CACHE_TTL and names this host.

    If the server refuses to mint a token, use the browser token but treat it
    as valid for no longer than CACHE_TTL. On success, revoke the original.
    """
    token = issued["access_token"]
    try:
        resp = _anonymous().post(
            f"{root}/access/api/v1/tokens",
            headers={"Authorization": f"Bearer {token}"},
            json={
                "scope": "applied-permissions/user",
                "expires_in": int(CACHE_TTL),
                "description": f"ironside release download on {socket.gethostname()}",
            },
            timeout=30,
        )
        if resp.ok:
            body = resp.json()
            minted = (
                body["access_token"],
                float(body.get("expires_in", CACHE_TTL)),
                body.get("token_id", ""),
            )
            if issued.get("token_id") and _revoke(root, minted[0], issued["token_id"]):
                log.debug("revoked the token the sign-in issued")
            return minted
        log.debug(
            "could not mint a token (HTTP %s). Using the issued one", resp.status_code
        )
    except (requests.RequestException, ValueError, KeyError) as exc:
        log.debug("could not mint a token (%s). Using the issued one", exc)
    return token, float(CACHE_TTL), ""


def _confirm_open() -> None:
    """Wait for confirmation before the browser takes focus."""
    print(
        "Press Enter to open the browser, or Ctrl-C to cancel: ",
        end="",
        file=sys.stderr,
        flush=True,
    )
    try:
        input()
    except EOFError:
        print(file=sys.stderr)
    except KeyboardInterrupt:
        print(file=sys.stderr)
        raise AuthError("sign-in cancelled") from None


def web_login(root: str, timeout: float = LOGIN_TIMEOUT, confirm: bool = None) -> str:
    """Sign in through a browser and return an access token.

    The URL is printed so it can be opened on another machine.
    """
    if confirm is None:
        confirm = sys.stdin.isatty()

    session_id = str(uuid.uuid4())
    log.info("signing in to %s through a browser", urlparse(root).hostname)
    log.info("if the browser asks for a code, enter: %s", session_id[-4:])
    if confirm:
        _confirm_open()

    anon = _anonymous()
    url = f"{root}/access/api/v2/authentication/jfrog_client_login/request"
    try:
        resp = anon.post(url, json={"session": session_id}, timeout=30)
    except requests.RequestException as exc:
        raise AuthError(f"cannot reach {url}: {exc}") from exc
    if not resp.ok:
        raise AuthError(
            f"browser sign-in failed with HTTP {resp.status_code}. "
            "Artifactory 7.64 or newer is required. Pass --auth-token instead"
        )

    # jfClientCode=1 selects the login protocol. The browser may ask for the
    # final four characters of the session UUID printed above.
    login_url = (
        f"{root}/ui/login?jfClientSession={session_id}"
        f"&jfClientName={quote(CLIENT_NAME)}&jfClientCode=1"
    )
    log.info("open this URL: %s", login_url)
    try:
        webbrowser.open(login_url)
    except Exception:
        log.debug("could not open a browser. Open the URL above manually")

    poll = f"{root}/access/api/v2/authentication/jfrog_client_login/token/{session_id}"
    deadline = time.monotonic() + timeout
    log.info("waiting for sign-in")
    while time.monotonic() < deadline:
        try:
            resp = anon.get(poll, timeout=30)
        except requests.RequestException as exc:
            log.debug("poll failed, retrying: %s", exc)
            resp = None
        if resp is not None and resp.ok:
            body = resp.json()
            token, ttl, token_id = _bounded(root, body)
            # Keep the old token until a replacement is stored.
            stale = _cache_entry(root)
            _cache_write(root, token, ttl, body.get("username", ""), token_id)
            if stale and stale.get("token_id") and _revoke(root, token, stale["token_id"]):
                log.debug("revoked the token this sign-in replaces")
            log.info("signed in as %s", body.get("username", "?"))
            return token
        time.sleep(POLL_INTERVAL)
    raise AuthError(f"sign-in timed out after {int(timeout)}s. Run again or pass --auth-token")


# --------------------------------------------------------------------------- #
# What callers use
# --------------------------------------------------------------------------- #


def authorize(
    session: requests.Session,
    base_url: str,
    token: str = None,
    login: bool = False,
    interactive: bool = None,
) -> str:
    """Set session credentials and return their source.

    login forces a fresh browser sign-in. interactive defaults to whether stdin
    and stdout are terminals.
    """
    root = platform_root(base_url)
    if interactive is None:
        interactive = sys.stdin.isatty() and sys.stdout.isatty()

    if login:
        # Explicit login also works remotely because the URL is printed.
        session.auth = bearer(web_login(root, confirm=interactive))
        return "a browser sign-in"

    if token:
        session.auth = bearer(token)
        return "--auth-token"

    env_token = os.environ.get(TOKEN_ENV)
    if env_token:
        session.auth = bearer(env_token)
        return f"${TOKEN_ENV}"

    cached = _cache_read(root)
    if cached:
        session.auth = bearer(cached)
        return f"the OS keyring ({SERVICE}/{_account(root)})"

    # Leave session.auth unset so requests can apply netrc credentials.
    if requests.utils.get_netrc_auth(base_url):
        return "~/.netrc"

    if interactive:
        session.auth = bearer(web_login(root, confirm=interactive))
        return "a browser sign-in"

    return "no credentials"
