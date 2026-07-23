#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>
# SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

"""Scaffold a per-project integration tree under trees/.

Files are written as *.in templates; the build substitutes
@KDE_INSTALL_*@ macros so paths match the configured install dirs
(e.g. libexecdir is /usr/lib on Arch).

Example:
    new-tree.py --name kjar \\
        --app-id org.kde.kjar \\
        --display-name "Java Support" \\
        --icon application-x-java-archive \\
        --remote-name kjar-nightly \\
        --remote-url https://cdn.kde.org/flatpak/kjar-nightly/kjar-nightly.flatpakrepo \\
        --post-install "flatpak run org.kde.kjar --generate-wrappers" \\
        --take-over-mime-types \\
        --cmd java --cmd javac \\
        --mime application/java-archive \\
        --binfmt jar --binfmt-cmd "java -jar" \\
        [--output-dir trees]
"""

import argparse
import sys
from pathlib import Path

# REUSE-IgnoreStart
SPDX_HEADER: str = (
    "# SPDX-License-Identifier: CC0-1.0\n"
    "# SPDX-FileCopyrightText: NONE\n"
)
# REUSE-IgnoreEnd


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--name", required=True, help="Tree name under trees/")
    parser.add_argument("--app-id", required=True, help="Flatpak application ID")
    parser.add_argument("--display-name", help="Human-readable name (defaults to --name)")
    parser.add_argument("--icon", default="application-x-executable", help="Icon name")
    parser.add_argument("--remote-name", required=True, help="Flatpak remote name")
    parser.add_argument("--remote-url", required=True, help="Flatpak .flatpakrepo URL")
    parser.add_argument("--post-install", help="Command run after a successful install")
    parser.add_argument(
        "--take-over-mime-types",
        action="store_true",
        help="Make the Flatpak the default handler for configured MIME types after installation",
    )
    parser.add_argument(
        "--cmd",
        action="append",
        default=[],
        dest="cmds",
        metavar="CMD",
        help="Command to shim into usr/bin (repeatable)",
    )
    parser.add_argument("--mime", help="MIME type to register a handler for")
    parser.add_argument("--binfmt", metavar="EXT", help="File extension for a binfmt rule")
    parser.add_argument("--binfmt-cmd", help="Interpreter command for the binfmt rule")
    parser.add_argument("--output-dir", default="trees", help="Where to create the tree")
    return parser.parse_args(argv)


def write_file(path: Path, content: str, executable: bool = False) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
    if executable:
        path.chmod(0o755)


def installer_config(args: argparse.Namespace, display_name: str) -> str:
    lines: list[str] = [
        SPDX_HEADER.rstrip("\n"),
        "[App]",
        f"Id={args.app_id}",
        f"Name={display_name}",
        f"Icon={args.icon}",
    ]
    if args.mime:
        lines.append(f"MimeTypes={args.mime}")
    lines += [
        "",
        "[Remote]",
        f"Name={args.remote_name}",
        f"Url={args.remote_url}",
    ]
    if args.post_install or args.take_over_mime_types:
        lines += ["", "[Install]"]
        if args.post_install:
            lines.append(f"PostInstall={args.post_install}")
        if args.take_over_mime_types:
            lines.append("TakeOverMimeTypes=true")
    return "\n".join(lines) + "\n"


def command_shim(name: str, cmd: str) -> str:
    return (
        "#!/usr/bin/env bash\n"
        f"{SPDX_HEADER}"
        f'exec @KDE_INSTALL_FULL_LIBEXECDIR@/package-compatibility-helper-run {name} {cmd} "$@"\n'
    )


def desktop_entry(args: argparse.Namespace, display_name: str) -> str:
    return (
        f"{SPDX_HEADER}"
        "[Desktop Entry]\n"
        f"Name={display_name}\n"
        f"Comment=Open this file type with {display_name}\n"
        "Exec=@KDE_INSTALL_FULL_BINDIR@/package-compatibility-helper %f\n"
        f"Icon={args.icon}\n"
        "Terminal=false\n"
        "Type=Application\n"
        "NoDisplay=true\n"
        f"MimeType={args.mime};\n"
        "Categories=Development;\n"
    )


def binfmt_interpreter(args: argparse.Namespace) -> str:
    return (
        "#!/usr/bin/env bash\n"
        f"{SPDX_HEADER}"
        f'exec {args.binfmt_cmd} "$@"\n'
    )


def binfmt_rule(args: argparse.Namespace) -> str:
    return (
        f"{SPDX_HEADER}"
        f":PackageCompatibilityHelper-{args.name}:E::{args.binfmt}::@KDE_INSTALL_FULL_LIBEXECDIR@/{args.name}-binfmt:F\n"
    )


def main(argv: list[str]) -> int:
    args = parse_args(argv)

    if args.binfmt and not args.binfmt_cmd:
        print("--binfmt requires --binfmt-cmd", file=sys.stderr)
        return 1

    display_name: str = args.display_name or args.name
    root = Path(args.output_dir) / args.name

    write_file(
        root / "usr/share/package-compatibility-helper/apps" / f"{args.name}.conf.in",
        installer_config(args, display_name),
    )

    for cmd in args.cmds:
        write_file(root / "usr/bin" / f"{cmd}.in", command_shim(args.name, cmd), executable=True)

    if args.mime:
        # Downstream trees may be added at runtime, after the main helper's
        # desktop entry has been installed. Ship a dedicated MIME shim for
        # those trees so matching files can still open the helper.
        write_file(
            root / "usr/share/applications" / f"package-compatibility-helper-{args.name}.desktop.in",
            desktop_entry(args, display_name),
        )

    if args.binfmt:
        write_file(
            root / "usr/libexec" / f"{args.name}-binfmt.in",
            binfmt_interpreter(args),
            executable=True,
        )
        write_file(root / "usr/lib/binfmt.d" / f"{args.name}.conf.in", binfmt_rule(args))

    print(f"Created tree: {root}")
    print(f"Install it with -DPACKAGE_COMPATIBILITY_HELPER_TREES={args.name} (or 'all').")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
