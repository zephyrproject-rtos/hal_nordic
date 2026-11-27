# IronSide TE Client

The IronSide TE Client is the library that a **non-secure application** links against to call the **secure services** of IronSide TE.

## Client library contents

The client library directory contains the following headers and source files:

- `include/ironside_client/`: The headers that an application includes.
  There is one header for each service.
  The `error.h` header defines the status codes of IronSide TE, and `psa/` contains the wrappers for standard Platform Security Architecture (PSA).
- `include/ironside_client/internal/` and `src/client.c`: The files that an application does not include directly.
  They contain the client core, the application binary interface (ABI) and the PSA wire layer in `internal/psa/`.
  The ABI in `internal/client.h` defines the secure-gateway entry address, the service handles, the call layout, and the only call point, `ironside_client_call`.
- `src/services.c`: The implementation of the service calls other than PSA Crypto.
  The public headers only declare these functions, so an application builds this file with the rest of `src/`.
- `src/psa_crypto.c`: The PSA Crypto dispatch.
  The `include/ironside_client/psa/crypto_abi.h` header describes the layout that it expects for the objects passed to it.
- `include/ironside_client/integration.h`: The calling contract and the integration hooks, which are the two weak lock functions that the integrator of a real-time operating system (RTOS) overrides.
- `include/ironside_client/layout.h` and `image.h`: The application view of the flash and RAM map and of the image header.
- `include/ironside_client/feature_level.h`: `IRONSIDE_CLIENT_SUPPORTED_FEATURE_LEVEL`, the highest IronSide TE feature level whose features this client supports.
- `psa_attest_token.cddl`: The Concise Data Definition Language (CDDL) format of the token that `psa_initial_attest_get_token()` returns.

For how to build against it and what it offers, see [Secure services and boot failure codes](../doc/secure-services.md) in the user guide.
