# scripts/

| Script | What it does | Needs |
| --- | --- | --- |
| `check-format.sh [--fix]` | Verifies (or applies) `clang-format` on `src/`, `test/` and `include/`. CI runs the check. | clang-format 23 (`pip install "clang-format~=23.1"`) |
| `coverage.sh [--html]` | Runs the native tests instrumented with gcov and fails below 100 % line / 70 % branch coverage of `src/core`, `src/hardware`, `src/application`. CI runs it. | PlatformIO, gcovr (`pip install gcovr`) |
| `build_docs.py` | Renders `docs/diagrams/*.mmd` and builds `docs/generated/coffee-sentinel-architecture.pdf` from `docs/architecture.md`. | pandoc, a LaTeX engine (or wkhtmltopdf / weasyprint), Mermaid CLI via `mmdc`, `npx` or Docker |
| `generate_demo_gif.py` | Draws the six-frame `docs/images/coffee-flow.gif` used by the README. | Pillow (`pip install pillow`) |

All scripts are also reachable from the `Makefile` (`make check-format`, `make docs`, `make gif`).
