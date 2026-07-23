from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent.parent
TARGETS_PATH = ROOT / "tools" / "release_targets.json"


def load_targets() -> dict[str, Any]:
    return json.loads(TARGETS_PATH.read_text(encoding="utf-8"))


def target_names(config: dict[str, Any], enabled_only: bool = False) -> list[str]:
    targets = config.get("targets") or {}
    names = []
    for name, target in targets.items():
        if enabled_only and not target.get("enabled", False):
            continue
        names.append(name)
    return sorted(names)


def print_targets(config: dict[str, Any], enabled_only: bool = False) -> None:
    for name in target_names(config, enabled_only=enabled_only):
        target = config["targets"][name]
        state = "enabled" if target.get("enabled") else "disabled"
        display = target.get("display_name", name)
        env = target.get("platformio_env", "")
        print(f"{name}\t{state}\t{display}\t{env}")


def resolve_path(value: str | Path) -> Path:
    path = Path(value)
    return path if path.is_absolute() else ROOT / path


def package_path(path: Path, root: Path) -> str:
    return str(path.relative_to(root)).replace("\\", "/")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def manifest_entry(path: Path, source: str, out_dir: Path) -> dict[str, Any]:
    return {
        "path": package_path(path, out_dir),
        "source": source,
        "bytes": path.stat().st_size,
        "sha256": sha256(path),
    }


def copy_with_manifest_entry(source: Path, destination: Path, out_dir: Path) -> dict[str, Any]:
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
    return manifest_entry(destination, package_path(source, ROOT), out_dir)


def render_embedded_python_script(
    source: Path,
    python_source: Path,
    destination: Path,
    out_dir: Path,
    marker: str = "{{MISTER_SERIAL_BRIDGE_PYTHON}}",
) -> dict[str, Any]:
    template = source.read_text(encoding="utf-8")
    if marker not in template:
        raise SystemExit(f"Missing embed marker {marker!r} in {source}")
    embedded = python_source.read_text(encoding="utf-8").rstrip() + "\n"
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(template.replace(marker, embedded), encoding="utf-8", newline="\n")
    return manifest_entry(
        destination,
        f"generated:{package_path(source, ROOT)}+{package_path(python_source, ROOT)}",
        out_dir,
    )


def run(command: list[str]) -> None:
    subprocess.run(command, cwd=ROOT, check=True)


def run_capture(command: list[str]) -> str:
    completed = subprocess.run(
        command,
        cwd=ROOT,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    return completed.stdout.strip()


def expand_command(command: list[str], target_name: str, target: dict[str, Any]) -> list[str]:
    replacements = {
        "python": sys.executable,
        "target": target_name,
        "env": target.get("platformio_env", target_name),
    }
    expanded: list[str] = []
    for part in command:
        value = str(part)
        if value == "{python}":
            expanded.append(sys.executable)
        else:
            expanded.append(value.format(**replacements))
    return expanded


def run_configured_commands(commands: list[list[str]], target_name: str, target: dict[str, Any]) -> None:
    for command in commands:
        run(expand_command(command, target_name, target))


def build_target(target_name: str, target: dict[str, Any]) -> None:
    run_configured_commands(target.get("validation_commands", []), target_name, target)
    build_command = target.get("build_command") or ["pio", "run", "-e", "{env}"]
    run(expand_command(build_command, target_name, target))


def firmware_version_info(target: dict[str, Any]) -> dict[str, str]:
    header = resolve_path(target["version_header"])
    text = header.read_text(encoding="utf-8")
    match = re.search(r'#define\s+FIRMWARE_VERSION_STRING\s+"([^"]+)"', text)
    if not match:
        raise SystemExit(f"Missing FIRMWARE_VERSION_STRING in {header}")
    version = match.group(1)
    return {
        "version": version,
        "expected_tag": f"v{version}",
        "source": package_path(header, ROOT),
    }


def versioned_uf2_name(target: dict[str, Any], version: str) -> str:
    stem = target.get("firmware_artifact_stem", target.get("platformio_env", "firmware"))
    return f"{stem}_v{version}.uf2"


def git_metadata() -> dict[str, str]:
    def optional(command: list[str], default: str = "") -> str:
        try:
            return run_capture(command)
        except subprocess.CalledProcessError:
            return default

    latest_tag = optional(["git", "describe", "--tags", "--abbrev=0"])
    dirty_status = optional(["git", "status", "--porcelain", "--untracked-files=normal"])
    return {
        "branch": optional(["git", "rev-parse", "--abbrev-ref", "HEAD"], "unknown"),
        "commit": optional(["git", "rev-parse", "--short", "HEAD"], "unknown"),
        "exact_tag": optional(["git", "describe", "--tags", "--exact-match"], ""),
        "latest_tag": latest_tag,
        "dirty": "yes" if dirty_status else "no",
    }


def render_manual_pdf(target: dict[str, Any], out_dir: Path) -> dict[str, Any] | None:
    config = target.get("manual_pdf")
    if not config:
        return None

    source = resolve_path(config["source"])
    destination = out_dir / config.get("destination", "manual.pdf")
    destination.parent.mkdir(parents=True, exist_ok=True)
    command = [
        sys.executable,
        config.get("script", "tools/render_manual_pdf.py"),
        "--input",
        str(source),
        "--output",
        str(destination),
    ]
    if config.get("verify_render", True):
        command.append("--verify-render")
    run(command)
    return manifest_entry(destination, f"generated:{package_path(source, ROOT)}", out_dir)


def copy_package_files(target: dict[str, Any], out_dir: Path) -> list[dict[str, Any]]:
    entries: list[dict[str, Any]] = []
    for item in target.get("package_files", []):
        source = resolve_path(item["source"])
        if not source.exists():
            if item.get("required", True):
                raise SystemExit(f"Missing package file: {source}")
            continue
        destination = out_dir / item.get("destination", package_path(source, ROOT))
        if "embed_python_source" in item:
            python_source = resolve_path(item["embed_python_source"])
            if not python_source.exists():
                raise SystemExit(f"Missing embedded Python source: {python_source}")
            entries.append(render_embedded_python_script(
                source,
                python_source,
                destination,
                out_dir,
                item.get("embed_marker", "{{MISTER_SERIAL_BRIDGE_PYTHON}}"),
            ))
        else:
            entries.append(copy_with_manifest_entry(source, destination, out_dir))
    return entries


def render_manager_release_descriptor(
    firmware_path: Path,
    firmware_info: dict[str, str],
    out_dir: Path,
) -> dict[str, Any]:
    destination = out_dir / "adapt-manager-release.json"
    run([
        sys.executable,
        "tools/adapt_manager/generate_release_descriptor.py",
        "--uf2",
        str(firmware_path),
        "--version",
        firmware_info["version"],
        "--output",
        str(destination),
    ])
    return manifest_entry(destination, "generated:adapt_manager_release_descriptor", out_dir)


def run_latency_cycle(args: argparse.Namespace, target: dict[str, Any], out_dir: Path) -> tuple[Path | None, list[str] | None]:
    if not args.run_latency:
        return None, None
    latency = target.get("latency")
    if not latency:
        raise SystemExit(f"Target '{args.target}' has no latency runner configured")

    latency_dir = out_dir / latency.get("out_dir", "validation/latency_matrix")
    latency_dir.mkdir(parents=True, exist_ok=True)
    command = [
        sys.executable,
        latency["script"],
        "--inputs",
        args.latency_inputs or latency.get("inputs", "all"),
        "--outputs",
        args.latency_outputs or latency.get("outputs", "mister"),
        "--host",
        args.latency_host or latency.get("host", "internal"),
        "--count",
        str(args.latency_count or latency.get("count", 128)),
        "--out-dir",
        str(latency_dir),
    ]
    if args.latency_port:
        command.extend(["--port", args.latency_port])
    if args.latency_controller_in_loop:
        command.append("--controller-in-loop")
    if args.latency_trigger_target:
        command.extend(["--trigger-target", args.latency_trigger_target])
    if args.latency_notes:
        command.extend(["--notes", args.latency_notes])
    run(command)
    return latency_dir / latency.get("report", "latency_matrix_report.json"), command


def load_latency_report(report_path: Path | None) -> dict[str, Any] | None:
    if report_path is None or not report_path.exists():
        return None
    return json.loads(report_path.read_text(encoding="utf-8"))


def release_markdown(manifest: dict[str, Any],
                     target: dict[str, Any],
                     firmware_info: dict[str, str],
                     git_info: dict[str, str],
                     latency_report: dict[str, Any] | None,
                     latency_command: list[str] | None) -> str:
    display = target.get("display_name", manifest["target"])
    lines: list[str] = []
    lines.append(f"# {display} Release")
    lines.append("")
    lines.append(f"Generated UTC: {manifest['created_utc']}")
    lines.append(f"Target: {manifest['target']}")
    lines.append(f"Environment: {manifest['environment']}")
    lines.append(f"Firmware version: {firmware_info['version']}")
    lines.append(f"Expected Git tag: {firmware_info['expected_tag']}")
    lines.append(f"Branch: {git_info['branch']}")
    lines.append(f"Commit: {git_info['commit']}")
    lines.append(f"Working tree dirty: {git_info['dirty']}")
    lines.append(f"Exact tag: {git_info['exact_tag'] or '(not tagged)'}")
    if git_info["latest_tag"]:
        lines.append(f"Latest tag: {git_info['latest_tag']}")
    hardware = target.get("hardware_compatibility", {})
    if hardware:
        lines.append(f"Hardware compatibility: {hardware['description']}")
    lines.append("")

    lines.append("## Artifacts")
    lines.append("")
    lines.append("| File | Bytes | SHA256 |")
    lines.append("| --- | ---: | --- |")
    for entry in manifest["files"]:
        if entry["path"] == "release.md":
            continue
        lines.append(f"| `{entry['path']}` | {entry['bytes']} | `{entry['sha256']}` |")
    lines.append("")

    feature_matrix = target.get("feature_matrix", [])
    if feature_matrix:
        lines.append("## Feature Matrix")
        lines.append("")
        lines.append("| Area | Feature | Status | Notes |")
        lines.append("| --- | --- | --- | --- |")
        for row in feature_matrix:
            lines.append(f"| {row['area']} | {row['feature']} | {row['status']} | {row['notes']} |")
        lines.append("")

    if latency_command:
        lines.append("## Latency Validation")
        lines.append("")
        lines.append("Command:")
        lines.append("")
        lines.append("```")
        lines.append(" ".join(latency_command))
        lines.append("```")
        if latency_report:
            cases = latency_report.get("cases", [])
            failures = [case for case in cases if case.get("status") != "pass"]
            lines.append(f"Result: {len(cases)} cases, {len(failures)} failures.")
            for case in cases:
                usb = (case.get("latency_stats") or {}).get("d_usb") or {}
                if usb:
                    lines.append(
                        f"- {case.get('input')}/{case.get('output')}/{latency_report.get('host')}: "
                        f"avg {usb.get('avg_us')} us, p50 {usb.get('p50_us')} us, "
                        f"p95 {usb.get('p95_us')} us"
                    )
        else:
            lines.append("Result: latency command ran, but no JSON report was found.")
        lines.append("")

    return "\n".join(lines)


def package_target(args: argparse.Namespace, target: dict[str, Any]) -> int:
    firmware_info = firmware_version_info(target)
    git_info = git_metadata()
    if args.require_tag_match and git_info["exact_tag"] != firmware_info["expected_tag"]:
        raise SystemExit(
            f"Firmware version {firmware_info['version']} expects tag "
            f"{firmware_info['expected_tag']}, but HEAD tag is "
            f"{git_info['exact_tag'] or '(not tagged)'}"
        )
    if (args.require_clean or args.require_tag_match) and git_info["dirty"] != "no":
        raise SystemExit("Release packaging requires a clean working tree")

    if not args.skip_build:
        build_target(args.target, target)
    if args.build_only:
        return 0

    out_dir = resolve_path(args.out_dir or target.get("default_out_dir", f"dist/{args.target}_package"))
    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True)

    manifest: dict[str, Any] = {
        "created_utc": datetime.now(timezone.utc).isoformat(),
        "target": args.target,
        "environment": target.get("platformio_env", args.target),
        "firmware_version": firmware_info,
        "hardware_compatibility": target.get("hardware_compatibility", {}),
        "files": [],
    }

    uf2_source = resolve_path(target["uf2_source"])
    if not uf2_source.exists():
        raise SystemExit(f"Missing build artifact: {uf2_source}")
    packaged_uf2 = out_dir / versioned_uf2_name(target, firmware_info["version"])
    manifest["files"].append(copy_with_manifest_entry(
        uf2_source,
        packaged_uf2,
        out_dir,
    ))
    manifest["files"].append(render_manager_release_descriptor(
        packaged_uf2,
        firmware_info,
        out_dir,
    ))

    manual_entry = render_manual_pdf(target, out_dir)
    if manual_entry:
        manifest["files"].append(manual_entry)

    manifest["files"].extend(copy_package_files(target, out_dir))

    latency_report_path, latency_command = run_latency_cycle(args, target, out_dir)
    if args.require_latency and not latency_report_path:
        raise SystemExit("Latency validation is required; rerun with --run-latency")
    if latency_report_path:
        for path in sorted(latency_report_path.parent.rglob("*")):
            if path.is_file():
                manifest["files"].append(manifest_entry(path, "generated:latency_matrix", out_dir))

    release_path = out_dir / "release.md"
    release_path.write_text(
        release_markdown(
            manifest,
            target,
            firmware_info,
            git_info,
            load_latency_report(latency_report_path),
            latency_command,
        ),
        encoding="utf-8",
    )
    manifest["files"].append(manifest_entry(release_path, "generated:release_notes", out_dir))

    print(f"Package: {out_dir}")
    print(f"Release notes: {release_path}")
    for entry in manifest["files"]:
        print(f"{entry['path']} ({entry['bytes']} bytes)")
    return 0


def main(argv: list[str] | None = None) -> int:
    config = load_targets()
    parser = argparse.ArgumentParser(description="Build or package an Adapt release target")
    parser.add_argument("--target", default=config.get("default_target", "classic2usb"))
    parser.add_argument("--list-targets", action="store_true", help="List configured release targets and exit")
    parser.add_argument("--enabled-only", action="store_true", help="Only list enabled targets")
    parser.add_argument("--build-only", action="store_true", help="Run target validation/build without packaging")
    parser.add_argument("--out-dir", default="", help="Package output directory")
    parser.add_argument("--skip-build", action="store_true", help="Use existing target UF2 artifact")
    parser.add_argument("--require-tag-match", action="store_true",
                        help="Fail unless HEAD is tagged with v<FIRMWARE_VERSION_STRING>")
    parser.add_argument("--require-clean", action="store_true",
                        help="Fail unless the Git working tree is clean")
    parser.add_argument("--run-latency", action="store_true", help="Run target latency validation into the package")
    parser.add_argument("--require-latency", action="store_true",
                        help="Fail unless --run-latency produced a latency report")
    parser.add_argument("--latency-port", default="", help="Serial port for latency matrix")
    parser.add_argument("--latency-inputs", default="", help="Latency input list")
    parser.add_argument("--latency-outputs", default="", help="Latency output list")
    parser.add_argument("--latency-host", default="", help="Latency host")
    parser.add_argument("--latency-count", type=int, default=0)
    parser.add_argument("--latency-controller-in-loop", action="store_true")
    parser.add_argument("--latency-trigger-target", default="")
    parser.add_argument("--latency-notes", default="")
    args = parser.parse_args(argv)

    if args.list_targets:
        print_targets(config, enabled_only=args.enabled_only)
        return 0

    targets = config.get("targets") or {}
    if args.target not in targets:
        known = ", ".join(target_names(config)) or "(none)"
        raise SystemExit(f"Unknown release target '{args.target}'. Known targets: {known}")

    target = targets[args.target]
    if not target.get("enabled", False):
        raise SystemExit(f"Release target '{args.target}' is configured but disabled")

    return package_target(args, target)


if __name__ == "__main__":
    raise SystemExit(main())
