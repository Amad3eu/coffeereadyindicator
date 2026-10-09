#!/usr/bin/env python3
"""Build the Coffee Sentinel architecture document.

Pipeline
--------
1. Render every ``docs/diagrams/*.mmd`` with Mermaid CLI:
     - SVG  -> docs/images/diagrams/<name>.svg   (versioned; used by Markdown on GitHub)
     - PNG  -> docs/generated/diagrams/<name>.png (used by the PDF)
2. Rewrite ``docs/architecture.md`` so image links point at the PNGs and
   write the result to ``docs/generated/architecture.md``.
3. Run pandoc to produce ``docs/generated/coffee-sentinel-architecture.pdf``.

Dependencies
------------
* pandoc (https://pandoc.org) and a PDF engine: xelatex (recommended),
  pdflatex, wkhtmltopdf or weasyprint
  -- or Docker, which runs the ``pandoc/latex`` image instead (``--pandoc docker``)
* Mermaid CLI, found in this order:
    1. ``mmdc`` on PATH            (npm install -g @mermaid-js/mermaid-cli)
    2. ``npx @mermaid-js/mermaid-cli``
    3. Docker image ``minlag/mermaid-cli``
  Use ``--renderer`` to force one, or ``--skip-diagrams`` to reuse existing output.

Usage
-----
    python3 scripts/build_docs.py                 # full build
    python3 scripts/build_docs.py --diagrams-only # only render the .mmd files
    python3 scripts/build_docs.py --skip-diagrams # PDF from previously rendered diagrams
    python3 scripts/build_docs.py --pdf-engine wkhtmltopdf
    python3 scripts/build_docs.py --renderer docker --pandoc docker   # Docker only, nothing else installed
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"
DIAGRAMS = DOCS / "diagrams"
SVG_OUT = DOCS / "images" / "diagrams"
GENERATED = DOCS / "generated"
PNG_OUT = GENERATED / "diagrams"
SOURCE_MD = DOCS / "architecture.md"
GENERATED_MD = GENERATED / "architecture.md"
PDF_OUT = GENERATED / "coffee-sentinel-architecture.pdf"

MERMAID_CONFIG = {"theme": "neutral", "themeVariables": {"fontFamily": "DejaVu Sans, Helvetica, Arial, sans-serif"}}
PUPPETEER_CONFIG = {"args": ["--no-sandbox", "--disable-setuid-sandbox"]}
# Pinned: newer tags ship a Puppeteer that cannot find its bundled browser.
DOCKER_IMAGE = "minlag/mermaid-cli:11.4.2"
PANDOC_DOCKER_IMAGE = "pandoc/latex:3.6"
PDF_ENGINES = ("xelatex", "pdflatex", "wkhtmltopdf", "weasyprint")
HOST_FONT_DIRS = (Path("/usr/share/fonts"), Path("/usr/local/share/fonts"), Path.home() / ".fonts",
                  Path.home() / ".local/share/fonts")


def log(message: str) -> None:
    print(f"[build-docs] {message}", flush=True)


def fail(message: str) -> "NoReturn":  # type: ignore[name-defined]
    print(f"[build-docs] error: {message}", file=sys.stderr)
    sys.exit(1)


# --------------------------------------------------------------------------
# Mermaid rendering
# --------------------------------------------------------------------------
class MermaidRenderer:
    """Runs Mermaid CLI through whichever backend is available."""

    def __init__(self, preference: str) -> None:
        self.backend = self._detect(preference)
        log(f"mermaid renderer: {self.backend}")
        self._tmp = Path(tempfile.mkdtemp(prefix="coffee-docs-"))
        self.config_file = self._tmp / "mermaid.json"
        self.puppeteer_file = self._tmp / "puppeteer.json"
        self.config_file.write_text(json.dumps(MERMAID_CONFIG))
        self.puppeteer_file.write_text(json.dumps(PUPPETEER_CONFIG))

    @staticmethod
    def _detect(preference: str) -> str:
        candidates = [preference] if preference != "auto" else ["mmdc", "npx", "docker"]
        for candidate in candidates:
            if candidate == "mmdc" and shutil.which("mmdc"):
                return "mmdc"
            if candidate == "npx" and shutil.which("npx"):
                return "npx"
            if candidate == "docker" and shutil.which("docker"):
                return "docker"
        fail(
            "no Mermaid renderer found. Install one of:\n"
            "  npm install -g @mermaid-js/mermaid-cli   (mmdc)\n"
            "  npx (comes with Node.js)\n"
            "  docker (uses the minlag/mermaid-cli image)\n"
            "or pass --skip-diagrams to reuse previously rendered files."
        )

    def render(self, source: Path, output: Path, scale: int = 2) -> None:
        output.parent.mkdir(parents=True, exist_ok=True)
        if self.backend == "docker":
            self._render_docker(source, output, scale)
            return
        command = ["mmdc"] if self.backend == "mmdc" else ["npx", "--yes", "@mermaid-js/mermaid-cli"]
        command += [
            "-i", str(source),
            "-o", str(output),
            "-c", str(self.config_file),
            "-p", str(self.puppeteer_file),
            "-b", "white",
            "-s", str(scale),
        ]
        run(command)

    def _render_docker(self, source: Path, output: Path, scale: int) -> None:
        # The image runs as its own unprivileged user, so it cannot write into
        # the project tree. It renders into a world-writable scratch directory
        # (which also holds the config files) and the result is copied over.
        scratch = Path(tempfile.mkdtemp(prefix="coffee-mmdc-"))
        scratch.chmod(0o777)
        try:
            shutil.copy(source, scratch / source.name)
            shutil.copy(self.config_file, scratch / "mermaid.json")
            for path in scratch.iterdir():
                path.chmod(0o666)
            # No "-p": the image ships its own Puppeteer config pointing at the
            # bundled Chromium, and overriding it breaks the browser lookup.
            run([
                "docker", "run", "--rm",
                "-v", f"{scratch}:/work",
                DOCKER_IMAGE,
                "-i", f"/work/{source.name}",
                "-o", f"/work/{output.name}",
                "-c", "/work/mermaid.json",
                "-b", "white",
                "-s", str(scale),
            ])
            shutil.copy(scratch / output.name, output)
        finally:
            shutil.rmtree(scratch, ignore_errors=True)


def run(command: list[str]) -> None:
    log("$ " + " ".join(command))
    result = subprocess.run(command, cwd=ROOT)
    if result.returncode != 0:
        fail(f"command failed with exit code {result.returncode}")


def render_diagrams(renderer: MermaidRenderer, formats: tuple[str, ...]) -> list[Path]:
    sources = sorted(DIAGRAMS.glob("*.mmd"))
    if not sources:
        fail(f"no .mmd files in {DIAGRAMS}")
    outputs: list[Path] = []
    for source in sources:
        if "svg" in formats:
            target = SVG_OUT / f"{source.stem}.svg"
            renderer.render(source, target, scale=1)
            outputs.append(target)
        if "png" in formats:
            target = PNG_OUT / f"{source.stem}.png"
            renderer.render(source, target, scale=2)
            outputs.append(target)
    return outputs


# --------------------------------------------------------------------------
# Markdown preparation
# --------------------------------------------------------------------------
IMAGE_LINK = re.compile(r"\]\(images/diagrams/([\w\-]+)\.svg\)")
RELATIVE_LINK = re.compile(r"\]\((?!https?://|#|generated/)([\w\-./]+\.md)(#[\w\-]+)?\)")
TALL_IMAGE_RATIO = 1.2     # height / width above which a diagram gets a height cap
TALL_IMAGE_HEIGHT = "58%"  # of the text height, leaves room for the caption and text


def png_size(path: Path) -> tuple[int, int]:
    """Width and height from the PNG header (no imaging library needed)."""
    with path.open("rb") as handle:
        header = handle.read(24)
    if header[:8] != b"\x89PNG\r\n\x1a\n":
        return (0, 0)
    return int.from_bytes(header[16:20], "big"), int.from_bytes(header[20:24], "big")


def png_link(match: "re.Match[str]") -> str:
    """Rewrites an SVG link to the rendered PNG, capping the height of tall
    diagrams so LaTeX does not push them onto a page of their own."""
    name = match.group(1)
    width, height = png_size(PNG_OUT / f"{name}.png")
    attributes = f"{{height={TALL_IMAGE_HEIGHT}}}" if width and height / width > TALL_IMAGE_RATIO else ""
    return f"](diagrams/{name}.png){attributes}"


def prepare_markdown(use_png: bool) -> Path:
    text = SOURCE_MD.read_text(encoding="utf-8")
    if use_png:
        text = IMAGE_LINK.sub(png_link, text)
    else:
        text = IMAGE_LINK.sub(r"](../images/diagrams/\1.svg)", text)
    # Links to sibling Markdown files become links to the repository.
    text = RELATIVE_LINK.sub(r"](https://github.com/Amad3eu/coffeindicator/blob/main/docs/\1\2)", text)
    GENERATED.mkdir(parents=True, exist_ok=True)
    GENERATED_MD.write_text(text, encoding="utf-8")
    return GENERATED_MD


# --------------------------------------------------------------------------
# PDF
# --------------------------------------------------------------------------
def pick_pandoc(preference: str) -> str:
    if preference == "docker" or (preference == "auto" and shutil.which("pandoc") is None):
        if shutil.which("docker") is None:
            fail("neither pandoc nor docker found (https://pandoc.org/installing.html)")
        return "docker"
    if shutil.which("pandoc") is None:
        fail("pandoc not found (https://pandoc.org/installing.html)")
    return "local"


def pick_pdf_engine(preference: str) -> str:
    if preference != "auto":
        if shutil.which(preference) is None:
            fail(f"PDF engine '{preference}' not found on PATH")
        return preference
    for engine in PDF_ENGINES:
        if shutil.which(engine):
            return engine
    fail("no PDF engine found; install texlive-xetex (xelatex), wkhtmltopdf or weasyprint")


def find_host_font_dir() -> Path | None:
    """Directory containing DejaVuSans.ttf on this machine, if any."""
    for base in HOST_FONT_DIRS:
        if base.is_dir():
            for candidate in base.rglob("DejaVuSans.ttf"):
                return candidate.parent
    return None


def font_variables(pandoc: str, engine: str) -> list[str]:
    """xelatex font settings. DejaVu covers the arrows, box-drawing and Greek
    characters used in the docs; without it LaTeX falls back to Latin Modern
    and merely warns about missing glyphs."""
    if engine != "xelatex":
        return []
    if pandoc == "docker":
        # Fonts are looked up by file name through OSFONTDIR (see build_pdf).
        if find_host_font_dir() is None:
            log("DejaVu fonts not found on the host; the PDF uses Latin Modern")
            return []
        return [
            "-V", "mainfont=DejaVuSans.ttf",
            "-V", "mainfontoptions=BoldFont=DejaVuSans-Bold.ttf",
            "-V", "mainfontoptions=ItalicFont=DejaVuSans-Oblique.ttf",
            "-V", "mainfontoptions=BoldItalicFont=DejaVuSans-BoldOblique.ttf",
            "-V", "monofont=DejaVuSansMono.ttf",
            "-V", "monofontoptions=BoldFont=DejaVuSansMono-Bold.ttf",
        ]
    if shutil.which("fc-list") and "DejaVu Sans" in subprocess.run(
            ["fc-list", ":family"], capture_output=True, text=True).stdout:
        return ["-V", "mainfont=DejaVu Sans", "-V", "monofont=DejaVu Sans Mono"]
    log("DejaVu Sans not installed; the PDF uses Latin Modern")
    return []


def build_pdf(markdown: Path, pandoc: str, engine: str) -> None:
    if pandoc == "docker":
        prefix = [
            "docker", "run", "--rm",
            "-u", f"{os.getuid()}:{os.getgid()}",
            "-v", f"{ROOT}:/data", "-w", "/data",
        ]
        host_fonts = find_host_font_dir()
        if host_fonts is not None:
            prefix += ["-v", f"{host_fonts}:/fonts:ro", "-e", "OSFONTDIR=/fonts"]
        prefix.append(PANDOC_DOCKER_IMAGE)
        markdown_arg = str(markdown.relative_to(ROOT))
        output_arg = str(PDF_OUT.relative_to(ROOT))
        resource_arg = str(GENERATED.relative_to(ROOT))
    else:
        prefix = ["pandoc"]
        markdown_arg, output_arg, resource_arg = str(markdown), str(PDF_OUT), str(GENERATED)
    command = prefix + [
        markdown_arg,
        "-o", output_arg,
        # pandoc's own Markdown: wraps wide pipe-table cells and accepts image attributes.
        "--from", "markdown+pipe_tables+yaml_metadata_block+link_attributes+task_lists",
        "--pdf-engine", engine,
        "--toc", "--toc-depth=2",
        "--number-sections",
        "--resource-path", resource_arg,
        "-V", "geometry:margin=2.2cm",
        "-V", "colorlinks=true",
        "-V", "linkcolor=NavyBlue",
        "-V", "urlcolor=NavyBlue",
    ]
    command += font_variables(pandoc, engine)
    run(command)
    log(f"PDF written to {PDF_OUT.relative_to(ROOT)}")


# --------------------------------------------------------------------------
def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--diagrams-only", action="store_true", help="render diagrams and stop")
    parser.add_argument("--skip-diagrams", action="store_true", help="do not render diagrams; reuse existing PNGs")
    parser.add_argument("--renderer", choices=("auto", "mmdc", "npx", "docker"), default="auto")
    parser.add_argument("--pandoc", choices=("auto", "local", "docker"), default="auto",
                        help="use the local pandoc or the pandoc/latex Docker image (auto: local if installed)")
    parser.add_argument("--pdf-engine", choices=("auto",) + PDF_ENGINES, default="auto")
    parser.add_argument("--formats", default="svg,png", help="comma separated: svg, png (default: both)")
    args = parser.parse_args()

    formats = tuple(f.strip() for f in args.formats.split(",") if f.strip())

    if not args.skip_diagrams:
        renderer = MermaidRenderer(args.renderer)
        for output in render_diagrams(renderer, formats):
            log(f"rendered {output.relative_to(ROOT)}")
    if args.diagrams_only:
        return

    missing = [p for p in DIAGRAMS.glob("*.mmd") if not (PNG_OUT / f"{p.stem}.png").exists()]
    if missing:
        fail("missing rendered diagrams: " + ", ".join(p.stem for p in missing) + " (drop --skip-diagrams)")

    markdown = prepare_markdown(use_png=True)
    pandoc = pick_pandoc(args.pandoc)
    engine = "xelatex" if pandoc == "docker" else pick_pdf_engine(args.pdf_engine)
    build_pdf(markdown, pandoc, engine)


if __name__ == "__main__":
    main()
