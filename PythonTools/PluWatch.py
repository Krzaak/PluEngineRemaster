#!/usr/bin/env python3
"""PluWatch - dev file watcher for the PluEngine reflection/CMake pipeline.

Watches engine source directories and, on change, eagerly does the two things
CLion + CMake do too late to be useful:

  * runs the reflection generator when a header with a PLU_ macro changes, and
  * reconfigures the CMake build dir when the set of source / generated files
    changes (new/removed file, new/removed reflected class).

This front-loads reflection generation to *save time* rather than build time,
so by the time you hit Build everything is already generated and in the build
graph - no more "reflection changes only show up on the next build" lag.

Dependency-free (stdlib only): polls mtimes on an interval instead of using
watchdog/inotify, which keeps it identical on Linux and Windows and needs
nothing installed in the venv.

Usage (run with the build venv's python from the project root):
    Python/venv-linux/bin/python PythonTools/PluWatch.py --editor
    Python/venv-linux/bin/python PythonTools/PluWatch.py --runtime
    Python/venv-linux/bin/python PythonTools/PluWatch.py            # asks

See CLAUDE.md ("Reflection Code Generation") for the generator contract.
"""

import argparse
import os
import re
import subprocess
import sys
import time

# --- paths ------------------------------------------------------------------

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
GENERATOR = os.path.join(SCRIPT_DIR, "ReflectionGeneratorRegex.py")
REFLECTION_CACHE = os.path.join(PROJECT_ROOT, "ReflectionCache")

# --- what to watch ----------------------------------------------------------

# Source extensions relevant to CMake globs (add/remove -> reconfigure).
SOURCE_EXTS = {".cpp", ".cc", ".cxx", ".h", ".hpp", ".hh", ".inl"}
# Header extensions that can carry PLU_ reflection macros.
HEADER_EXTS = {".h", ".hpp", ".hh", ".inl"}

# Directory names to never descend into.
IGNORED_DIR_NAMES = {".git", ".idea", "__pycache__", "ReflectionCache"}
# Path fragments to ignore anywhere (build dirs, venvs, thirdparty).
IGNORED_FRAGMENTS = ("cmake-build-", "venv-", os.path.join("ThirdParty", ""))

PLU_MACRO_RE = re.compile(r"\bPLU_(CLASS|STRUCT|ENUM|PROPERTY|FUNCTION)\b")

# --- target / preset resolution ---------------------------------------------

# Which extra project (besides always-on LibEngine) each target adds, and the
# CMake configure preset + resulting build dir it maps to.
IS_WINDOWS = sys.platform.startswith("win")
PLATFORM = "Windows" if IS_WINDOWS else "Linux"


def preset_for(target: str, build_type: str = "Debug") -> str:
    # e.g. PluDebugLinux-Editor
    return f"Plu{build_type}{PLATFORM}-{target.capitalize()}"


# Roots watched per target. Each entry: (absolute dir, reflection project name).
def watch_roots(target: str):
    roots = [
        (os.path.join(PROJECT_ROOT, "LibEngine", "include"), "LibEngine"),
        (os.path.join(PROJECT_ROOT, "LibEngine", "src"), "LibEngine"),
    ]
    if target == "editor":
        roots.append((os.path.join(PROJECT_ROOT, "Editor"), "Editor"))
    elif target == "runtime":
        roots.append((os.path.join(PROJECT_ROOT, "Runtime"), "Runtime"))
    return [(d, p) for d, p in roots if os.path.isdir(d)]


# --- pretty logging ---------------------------------------------------------

_USE_COLOR = sys.stdout.isatty() and not IS_WINDOWS


def _c(code: str, text: str) -> str:
    return f"\033[{code}m{text}\033[0m" if _USE_COLOR else text


def log(kind: str, msg: str):
    stamp = time.strftime("%H:%M:%S")
    colors = {"info": "36", "gen": "35", "cfg": "33", "warn": "31", "ok": "32"}
    tag = _c(colors.get(kind, "0"), f"[{kind}]")
    print(f"{_c('90', stamp)} {tag} {msg}", flush=True)


# --- filesystem snapshotting ------------------------------------------------

def _is_ignored(path: str) -> bool:
    return any(frag in path for frag in IGNORED_FRAGMENTS)


def snapshot_sources(roots):
    """Map absolute source-file path -> mtime for every watched source file."""
    snap = {}
    for root, _ in roots:
        for dirpath, dirnames, filenames in os.walk(root):
            dirnames[:] = [
                d for d in dirnames
                if d not in IGNORED_DIR_NAMES and not _is_ignored(os.path.join(dirpath, d))
            ]
            for name in filenames:
                if os.path.splitext(name)[1].lower() not in SOURCE_EXTS:
                    continue
                full = os.path.join(dirpath, name)
                try:
                    snap[full] = os.stat(full).st_mtime
                except OSError:
                    pass
    return snap


def project_of(path: str, roots) -> str:
    for root, project in roots:
        if path.startswith(root + os.sep):
            return project
    return "LibEngine"


def reflection_file_set(project: str):
    """Set of generated file names in ReflectionCache/<project> (+ shared cpp)."""
    files = set()
    proj_dir = os.path.join(REFLECTION_CACHE, project)
    for base in (proj_dir, REFLECTION_CACHE):
        if not os.path.isdir(base):
            continue
        try:
            for name in os.listdir(base):
                if name.endswith((".cpp", ".h")):
                    files.add(os.path.join(base, name))
        except OSError:
            pass
    return files


# --- actions ----------------------------------------------------------------

def run_generator(project: str) -> bool:
    log("gen", f"reflection generator -> {_c('1', project)}")
    proc = subprocess.run(
        [sys.executable, GENERATOR, "-bp", project],
        cwd=PROJECT_ROOT,
    )
    if proc.returncode != 0:
        log("warn", f"generator for {project} exited with {proc.returncode}")
        return False
    return True


def wipe_ninja_state(build_dir: str):
    """Work around the ninja dyndep assertion crash after a new PLU_CLASS."""
    for fname in (".ninja_deps", ".ninja_log"):
        p = os.path.join(build_dir, fname)
        if os.path.exists(p):
            try:
                os.remove(p)
                log("cfg", f"removed {fname} (dyndep workaround)")
            except OSError as e:
                log("warn", f"could not remove {fname}: {e}")


def reconfigure(preset: str, build_dir: str, wipe_ninja: bool):
    if wipe_ninja and os.path.isdir(build_dir):
        wipe_ninja_state(build_dir)
    log("cfg", f"cmake --preset {_c('1', preset)}")
    proc = subprocess.run(["cmake", "--preset", preset], cwd=PROJECT_ROOT)
    if proc.returncode != 0:
        log("warn", f"reconfigure exited with {proc.returncode}")
    else:
        log("ok", "reconfigured - CLion will reload if auto-reload is on")


# --- change handling --------------------------------------------------------

def handle_changes(added, removed, modified, roots, preset, build_dir):
    structural = bool(added or removed)

    # Which reflection projects need regenerating, and are we adding/removing
    # a reflected header (which can change the generated file set)?
    projects_to_gen = set()
    for path in list(added) + list(modified):
        if os.path.splitext(path)[1].lower() not in HEADER_EXTS:
            continue
        try:
            with open(path, "r", encoding="utf-8", errors="ignore") as f:
                if PLU_MACRO_RE.search(f.read()):
                    projects_to_gen.add(project_of(path, roots))
        except OSError:
            pass
    # Removed headers: we can't read them, so regenerate their project defensively.
    for path in removed:
        if os.path.splitext(path)[1].lower() in HEADER_EXTS:
            projects_to_gen.add(project_of(path, roots))

    need_reconfigure = structural
    new_generated_files = False

    for project in sorted(projects_to_gen):
        before = reflection_file_set(project)
        if run_generator(project):
            after = reflection_file_set(project)
            if after != before:
                need_reconfigure = True
                if after - before:
                    new_generated_files = True

    if need_reconfigure:
        reconfigure(preset, build_dir, wipe_ninja=new_generated_files)
    elif projects_to_gen:
        log("ok", "reflection regenerated (no reconfigure needed)")


# --- main loop --------------------------------------------------------------

def ask_target() -> str:
    while True:
        ans = input("Watch which target? [E]ditor / [R]untime: ").strip().lower()
        if ans in ("e", "editor"):
            return "editor"
        if ans in ("r", "runtime"):
            return "runtime"
        print("  please answer 'e' or 'r'")


def main():
    parser = argparse.ArgumentParser(description="PluEngine reflection/CMake dev watcher")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--editor", action="store_const", dest="target", const="editor")
    group.add_argument("--runtime", action="store_const", dest="target", const="runtime")
    parser.add_argument("--preset", help="override the CMake configure preset")
    parser.add_argument("--build-type", default="Debug",
                        help="Debug/Release/RelDbg (used when --preset is not given)")
    parser.add_argument("--interval", type=float, default=1.0,
                        help="poll interval in seconds (default 1.0)")
    args = parser.parse_args()

    target = args.target
    if target is None and args.preset is None:
        target = ask_target()
    elif target is None:
        target = "editor"  # preset given without target; only matters for watch roots

    preset = args.preset or preset_for(target, args.build_type)
    build_dir = os.path.join(PROJECT_ROOT, f"cmake-build-{args.build_type.lower()}-{target}")

    roots = watch_roots(target)
    if not roots:
        log("warn", "no watch roots found - is this the project root?")
        return 1

    log("info", f"target={_c('1', target)}  preset={_c('1', preset)}")
    for d, p in roots:
        log("info", f"watching {os.path.relpath(d, PROJECT_ROOT)}  -> {p}")
    if not os.path.isdir(build_dir):
        log("warn", f"build dir {os.path.relpath(build_dir, PROJECT_ROOT)} does not exist yet "
                    f"- configure it once before reconfigures can work")
    log("info", f"polling every {args.interval}s - Ctrl+C to stop")

    prev = snapshot_sources(roots)
    try:
        while True:
            time.sleep(args.interval)
            cur = snapshot_sources(roots)
            prev_keys, cur_keys = set(prev), set(cur)
            added = cur_keys - prev_keys
            removed = prev_keys - cur_keys
            modified = {p for p in prev_keys & cur_keys if cur[p] != prev[p]}

            if added or removed or modified:
                for p in sorted(added):
                    log("info", f"+ {os.path.relpath(p, PROJECT_ROOT)}")
                for p in sorted(removed):
                    log("info", f"- {os.path.relpath(p, PROJECT_ROOT)}")
                for p in sorted(modified):
                    log("info", f"~ {os.path.relpath(p, PROJECT_ROOT)}")
                handle_changes(added, removed, modified, roots, preset, build_dir)

            prev = cur
    except KeyboardInterrupt:
        print()
        log("info", "stopped")
    return 0


if __name__ == "__main__":
    sys.exit(main())
