from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_INPUT = ROOT / "docs" / "classic2usb" / "Classic2USB.md"
DEFAULT_OUTPUT = ROOT / "dist" / "Classic2USB.pdf"


CSS = """
<style>
@page {
  size: Letter;
  margin: 0.65in;
}
html {
  color: #1f2328;
  font-family: Arial, Helvetica, sans-serif;
  font-size: 10.5pt;
  line-height: 1.42;
}
body {
  max-width: 100%;
}
h1, h2, h3, h4 {
  color: #111827;
  line-height: 1.18;
  break-after: avoid;
}
h1 {
  font-size: 24pt;
  border-bottom: 2px solid #111827;
  padding-bottom: 0.14in;
}
h2 {
  font-size: 17pt;
  margin-top: 0.32in;
  border-bottom: 1px solid #d0d7de;
  padding-bottom: 0.06in;
}
h3 {
  font-size: 13pt;
  margin-top: 0.22in;
}
h4 {
  font-size: 11pt;
  margin-top: 0.18in;
}
p, ul, ol {
  margin-top: 0.06in;
  margin-bottom: 0.08in;
}
li {
  margin-bottom: 0.025in;
}
table {
  width: 100%;
  border-collapse: collapse;
  margin: 0.12in 0;
  break-inside: avoid;
}
th, td {
  border: 1px solid #d0d7de;
  padding: 4px 5px;
  vertical-align: top;
  word-break: break-word;
}
th {
  background: #f6f8fa;
  font-weight: 700;
}
code {
  font-family: Consolas, "Courier New", monospace;
  font-size: 0.92em;
  background: #f6f8fa;
  padding: 1px 3px;
  border-radius: 3px;
}
pre {
  white-space: pre-wrap;
  word-break: break-word;
  background: #f6f8fa;
  border: 1px solid #d0d7de;
  padding: 8px;
}
pre code {
  background: transparent;
  padding: 0;
}
a {
  color: #0969da;
}
blockquote {
  border-left: 4px solid #d0d7de;
  color: #57606a;
  margin-left: 0;
  padding-left: 0.14in;
}
hr {
  border: 0;
  border-top: 1px solid #d0d7de;
  margin: 0.22in 0;
}
</style>
"""


def find_executable(candidates: list[str]) -> str | None:
    for candidate in candidates:
        path = shutil.which(candidate)
        if path:
            return path
        literal = Path(candidate)
        if literal.exists():
            return str(literal)
    return None


def file_uri(path: Path) -> str:
    return path.resolve().as_uri()


def run(command: list[str]) -> None:
    subprocess.run(command, cwd=ROOT, check=True)


def render_manual_pdf(markdown_path: Path, output_path: Path, verify_render: bool) -> None:
    pandoc = find_executable(["pandoc"])
    if not pandoc:
        raise SystemExit("pandoc is required to render the target manual to PDF")

    browser = find_executable([
        "google-chrome",
        "chrome",
        "chromium",
        "chromium-browser",
        "msedge",
        r"C:\Program Files\Google\Chrome\Application\chrome.exe",
        r"C:\Program Files\Microsoft\Edge\Application\msedge.exe",
    ])
    if not browser:
        raise SystemExit("Chrome, Chromium, or Edge is required to print the target manual to PDF")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    if output_path.exists():
        output_path.unlink()

    temp_root = ROOT / "build" / "manual_pdf"
    temp_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="work-", dir=str(temp_root)) as tmp:
        tmp_dir = Path(tmp)
        style_path = tmp_dir / "manual_pdf_style.html"
        html_path = tmp_dir / "Classic2USB.html"
        browser_profile = tmp_dir / "chrome-profile"
        style_path.write_text(CSS, encoding="utf-8")

        run([
            pandoc,
            str(markdown_path),
            "--from=gfm",
            "--to=html5",
            "--standalone",
            "--metadata",
            "title=Classic2USB Firmware Manual",
            "--include-in-header",
            str(style_path),
            "--output",
            str(html_path),
        ])

        run([
            browser,
            "--headless",
            "--disable-gpu",
            "--no-sandbox",
            "--disable-extensions",
            "--allow-file-access-from-files",
            f"--user-data-dir={browser_profile}",
            f"--print-to-pdf={output_path.resolve()}",
            "--no-pdf-header-footer",
            file_uri(html_path),
        ])

        for _ in range(120):
            if output_path.exists() and output_path.stat().st_size > 0:
                break
            time.sleep(0.5)

        if not output_path.exists() or output_path.stat().st_size == 0:
            raise SystemExit(f"PDF render failed: {output_path}")

        if verify_render:
            pdftoppm = find_executable(["pdftoppm.exe", "pdftoppm"])
            if not pdftoppm:
                raise SystemExit("pdftoppm is required for PDF render verification")
            preview_prefix = tmp_dir / "manual-preview"
            run([
                pdftoppm,
                "-png",
                "-f",
                "1",
                "-l",
                "1",
                str(output_path),
                str(preview_prefix),
            ])
            previews = sorted(tmp_dir.glob("manual-preview*.png"))
            if not previews:
                raise SystemExit("PDF render verification did not produce a preview image")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Render a target manual Markdown file to a release PDF")
    parser.add_argument("--input", default=str(DEFAULT_INPUT), help="Markdown manual path")
    parser.add_argument("--output", default=str(DEFAULT_OUTPUT), help="Output PDF path")
    parser.add_argument("--verify-render", action="store_true", help="Render the first PDF page to PNG with Poppler")
    args = parser.parse_args(argv)

    render_manual_pdf(Path(args.input), Path(args.output), args.verify_render)
    print(f"Manual PDF: {Path(args.output)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
