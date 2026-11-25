# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Turn a draw.io SVG export into a small, diffable SVG.

draw.io renders every label as a <switch> holding an HTML <foreignObject> plus a
base64 PNG fallback. foreignObject is not rendered when an SVG is referenced from
an <img> (which is what Markdown does), so the fallback raster is what a reader
would actually see -- and it is what makes the export huge. This rewrites each
<switch> into real <text>/<tspan>, using the geometry draw.io already computed:
the wrapper div's margin-left / padding-top give the anchor point exactly.

Two steps, starting from the .drawio source next to the SVG:

    docker run --rm -e HOME=/tmp -v "$PWD:/data" rlespinasse/drawio-desktop-headless \
        -x -f svg -o /data/raw.svg /data/<name>.drawio
    python3 drawio_to_svg.py raw.svg <name>.svg

Check the result against the diagram as Confluence renders it before committing.
"""

import re
import sys
from html.parser import HTMLParser
from xml.sax.saxutils import escape


class LabelParser(HTMLParser):
    """Flatten draw.io's label HTML into lines of (text, bold, italic) runs."""

    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.lines = [[]]
        self.bold = 0
        self.italic = 0

    def handle_startendtag(self, tag, attrs):
        self.handle_starttag(tag, attrs)

    def handle_starttag(self, tag, attrs):
        if tag in ("b", "strong"):
            self.bold += 1
        elif tag in ("i", "em"):
            self.italic += 1
        elif tag == "br":
            self.lines.append([])
        elif tag in ("div", "p"):
            # A block opens a new line only when the current one already has
            # content, so <div><br></div> stays a single empty line.
            if self.lines[-1]:
                self.lines.append([])

    def handle_endtag(self, tag):
        if tag in ("b", "strong"):
            self.bold = max(0, self.bold - 1)
        elif tag in ("i", "em"):
            self.italic = max(0, self.italic - 1)

    def handle_data(self, data):
        if data:
            self.lines[-1].append((data, self.bold > 0, self.italic > 0))


def style_value(style, prop, default=None):
    m = re.search(r"(?:^|;)\s*" + re.escape(prop) + r"\s*:\s*([^;]+)", style)
    return m.group(1).strip() if m else default


def px(value, default=0.0):
    if value is None:
        return default
    m = re.match(r"(-?[\d.]+)", value)
    return float(m.group(1)) if m else default


def innermost_div(inner):
    """Return the contents of draw.io's innermost label div (the one carrying the
    font styling), skipping the flex/box-sizing wrappers around it."""
    start = inner.find('<div style="display: inline-block')
    if start < 0:
        return inner
    open_end = inner.index(">", start) + 1
    depth, i = 1, open_end
    while i < len(inner) and depth:
        nxt_open = inner.find("<div", i)
        nxt_close = inner.find("</div>", i)
        if nxt_close < 0:
            break
        if 0 <= nxt_open < nxt_close:
            depth += 1
            i = nxt_open + 4
        else:
            depth -= 1
            i = nxt_close + 6
    return inner[open_end : i - 6] if depth == 0 else inner[open_end:]


def convert_switch(block):
    fo = re.search(r"<foreignObject[^>]*>(.*)</foreignObject>", block, re.S)
    if not fo:
        return ""
    inner = fo.group(1)

    divs = re.findall(r'<div[^>]*style="([^"]*)"', inner)
    if len(divs) < 2:
        return ""
    outer_style, text_style = divs[0], divs[-1]

    left = px(style_value(outer_style, "margin-left"))
    top = px(style_value(outer_style, "padding-top"))
    width = px(style_value(outer_style, "width"))
    justify = style_value(outer_style, "justify-content", "center") or "center"
    align = style_value(outer_style, "align-items", "center") or "center"

    if "flex-end" in justify:
        x, anchor = left + width, "end"
    elif "flex-start" in justify:
        x, anchor = left, "start"
    else:
        x, anchor = left + width / 2.0, "middle"

    size = px(style_value(text_style, "font-size"), 12.0)
    family = style_value(text_style, "font-family", "Helvetica") or "Helvetica"
    family = family.split(",")[0].strip()
    colour = style_value(text_style, "color", "#000000") or "#000000"
    if colour.startswith("light-dark("):
        colour = colour[len("light-dark(") :].split(",")[0].strip()

    body = innermost_div(inner)
    parser = LabelParser()
    parser.feed(body)
    lines = [ln for ln in parser.lines]
    while lines and not any(t.strip() for t, _, _ in lines[-1]):
        lines.pop()
    if not lines:
        return ""

    line_height = size * 1.2
    n = len(lines)
    if "flex-start" in align:
        block_top = top
    elif "flex-end" in align:
        block_top = top - n * line_height
    else:
        block_top = top - n * line_height / 2.0
    # Each line is centred in its own line box; 0.36em lifts a baseline from a
    # box centre to where the glyphs actually sit.
    first = block_top + line_height / 2.0 + size * 0.36

    def fmt(n):
        return f"{n:g}"

    # draw.io puts a uniformly styled label's emphasis on the wrapper div rather
    # than in <i>/<b> tags, so it has to be picked up as a <text> default.
    base_italic = style_value(text_style, "font-style") == "italic"
    base_bold = (style_value(text_style, "font-weight") or "") in ("bold", "700")
    defaults = ""
    if base_bold:
        defaults += ' font-weight="bold"'
    if base_italic:
        defaults += ' font-style="italic"'

    out = [
        f'<text x="{fmt(x)}" y="{fmt(first)}" text-anchor="{anchor}" '
        f'font-family="{family}" font-size="{fmt(size)}"{defaults} fill="{colour}">'
    ]
    for i, runs in enumerate(lines):
        y = first + i * line_height
        parts = []
        for text, bold, italic in runs:
            if not text:
                continue
            attrs = ""
            if bold:
                attrs += ' font-weight="bold"'
            if italic:
                attrs += ' font-style="italic"'
            parts.append(f"<tspan{attrs}>{escape(text)}</tspan>" if attrs else escape(text))
        content = "".join(parts)
        out.append(f'<tspan x="{fmt(x)}" y="{fmt(y)}">{content}</tspan>')
    out.append("</text>")
    return "".join(out)


def clean(svg):
    svg = re.sub(r"<switch>.*?</switch>", lambda m: convert_switch(m.group(0)), svg, flags=re.S)
    svg = re.sub(r"<!DOCTYPE[^>]*>\s*", "", svg)
    svg = re.sub(r"<style[^>]*>.*?</style>", "", svg, flags=re.S)
    svg = re.sub(r'\s*style="[^"]*light-dark\([^"]*"', "", svg)
    svg = re.sub(r'\s*style="background:[^"]*"', "", svg)
    svg = re.sub(r'\s*pointer-events="[^"]*"', "", svg)
    svg = re.sub(r'\s*id="ge-svg-[^"]*"', "", svg)
    svg = re.sub(r"<defs\s*/>", "", svg)
    svg = re.sub(r"<g>\s*</g>", "", svg)
    # One element per line so the result diffs sensibly.
    svg = re.sub(r"><", ">\n<", svg)
    svg = re.sub(r"\n(</?tspan)", r"\1", svg)
    svg = re.sub(r"(</tspan>)\n(</text>)", r"\1\2", svg)
    svg = re.sub(r"\n{2,}", "\n", svg)
    return svg.strip() + "\n"


if __name__ == "__main__":
    src, dst = sys.argv[1], sys.argv[2]
    with open(src, encoding="utf-8") as fh:
        result = clean(fh.read())
    with open(dst, "w", encoding="utf-8") as fh:
        fh.write(result)
    print(f"{len(result):>7} bytes  {dst}")
