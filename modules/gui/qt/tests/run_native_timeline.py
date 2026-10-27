#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Stage and run real Windows PlayerController timeline tests in a fresh profile."""
import argparse
from hashlib import file_digest
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
import xml.etree.ElementTree as ET


def digest(path):
    with path.open("rb") as stream:
        return file_digest(stream, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", type=Path, required=True)
    parser.add_argument("--native-build", type=Path, required=True)
    parser.add_argument("--fixtures", type=Path, required=True)
    parser.add_argument("--core", type=Path, help="Matching rebuilt core DLL to test without changing the supplied package")
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    if os.name != "nt":
        parser.error("Run on Windows against the matching cross-built native test")
    package, build, fixtures, out = (path.resolve() for path in
        (args.package, args.native_build, args.fixtures, args.output_dir))
    if out == package or package in out.parents:
        parser.error("Use a new output directory outside the ordinary package")
    out.mkdir(parents=True, exist_ok=False)
    runtime, results = out / "runtime", out / "results"
    runtime.mkdir()
    results.mkdir()
    (runtime / "portable").mkdir()
    shutil.copytree(package / "plugins", runtime / "plugins",
                    ignore=shutil.ignore_patterns("plugins.dat"))
    program = "test_qt_player_timeline.exe"
    for name in ("libvlc.dll", "libvlccore.dll", "vlc-preparser.exe", "vlc-cache-gen.exe"):
        shutil.copyfile(package / name, runtime / name)
    if args.core:
        shutil.copyfile(args.core.resolve(), runtime / "libvlccore.dll")
    shutil.copyfile(build / program, runtime / program)
    env = {key: value for key, value in os.environ.items() if not key.startswith("VLC_")}
    env["VLC_USERDATA_PATH"] = str(results / "cache-data")
    cache = subprocess.run([str(runtime / "vlc-cache-gen.exe"), str(runtime / "plugins")],
        cwd=runtime, env=env, capture_output=True, timeout=60,
        creationflags=subprocess.CREATE_NO_WINDOW)
    (results / "cache-stderr.txt").write_bytes(cache.stderr)
    cache.check_returncode()
    env.update(VLC_TEST_PLUGIN_PATH=str(runtime / "plugins"),
               VLC_USERDATA_PATH=str(results / "userdata"),
               VLC_TIMELINE_FIXTURE_DIR=str(fixtures),
               VLC_TIMELINE_PYTHON=sys.executable,
               QT_QPA_PLATFORM="windows")
    command = [str(runtime / program), "-o", str(results / "qtest.txt") + ",txt",
               "-o", str(results / "qtest.xml") + ",xunitxml"]
    identities = {name: digest(runtime / name) for name in (program, "libvlc.dll", "libvlccore.dll")}
    fixture_identities = {path.name: digest(path) for path in fixtures.iterdir() if path.is_file()}
    (results / "launch.json").write_text(json.dumps({"command": command,
        "runtimeSha256": identities, "fixtureSha256": fixture_identities}, indent=2) + "\n")
    started = time.monotonic()
    timed_out = False
    with (results / "stdout.txt").open("wb") as stdout, (results / "stderr.txt").open("wb") as stderr:
        process = subprocess.Popen(command, cwd=runtime, env=env, stdout=stdout, stderr=stderr,
                                   creationflags=subprocess.CREATE_NO_WINDOW)
        (results / "pid.txt").write_text(str(process.pid))
        try:
            code = process.wait(timeout=90)
        except subprocess.TimeoutExpired:
            timed_out, code = True, process.poll()
    counts = {}
    if (results / "qtest.xml").exists():
        try:
            suite = ET.parse(results / "qtest.xml").getroot()
            counts = {key: int(suite.attrib[key]) for key in ("tests", "failures", "errors", "skipped")}
        except ET.ParseError:
            pass
    report = dict(exitCode=code, terminationConfirmed=process.poll() is not None,
                  timedOut=timed_out, forcedCleanupUsed=False, counts=counts,
                  elapsedSeconds=time.monotonic() - started, runtimeSha256=identities)
    (results / "verification.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report))
    assert not timed_out and code == 0 and report["terminationConfirmed"]
    assert counts == {"tests": 4, "failures": 0, "errors": 0, "skipped": 0}


if __name__ == "__main__":
    main()
