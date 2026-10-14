# SPDX-License-Identifier: GPL-2.0-or-later
"""Run the supplied Qt navigation fixture in one owned, bounded process."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--repo-root", type=Path, required=True)
parser.add_argument("--overlay", type=Path)
parser.add_argument("--mode", choices=["baseline", "fixed"], required=True)
parser.add_argument("--output-dir", type=Path, required=True)
args = parser.parse_args()
repo, output = args.repo_root.resolve(), args.output_dir.resolve()
assert not output.exists(), "Use a fresh output directory"
worker = Path(__file__).with_name("test_music_network_navigation.py")
expected = [
    "musicAlbumsActualAliasRestoresRow", "artistsAllActualAliasRestoresRow",
    "genresAllActualAliasRestoresRow", "genreAlbumsActualAliasRestoresRow",
    "musicPlaylistsAllActualAliasRestoresRow", "networkFoldersActualAliasRestoresRow",
    "networkDeviceActualAliasRestoresRow", "networkTreeActualAliasRestoresRow",
    "serviceRootActualAliasRestoresRow", "serviceBrowseActualAliasRestoresRow",
    "musicAlbumsOwnRouteThenForeignRouteWriter", "artistsAllOwnRouteThenForeignRouteWriter",
    "artistAlbumsOwnRouteThenForeignRouteWriter", "artistIdOwnRouteThenForeignRouteWriter",
    "genresAllOwnRouteThenForeignRouteWriter", "genreAlbumsOwnRouteThenForeignRouteWriter",
    "musicPlaylistsAllOwnRouteThenForeignRouteWriter", "musicPlaylistListOwnRouteThenForeignRouteWriter",
    "networkFoldersOwnRouteThenForeignRouteWriter", "networkDeviceOwnRouteThenForeignRouteWriter",
    "networkTreeOwnRouteThenForeignRouteWriter", "serviceRootOwnRouteThenForeignRouteWriter",
    "serviceBrowseOwnRouteThenForeignRouteWriter", "deviceConstructorBeforeLoaded",
    "deviceCountBeforeLoadingFalse", "deviceReadyModelReplacement", "deviceNoRequestAndUserViewSwitch",
]
def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

command = [sys.executable, str(worker), "--repo-root", str(repo),
    "--mode", args.mode, "--output-dir", str(output)]
if args.overlay:
    command += ["--overlay", str(args.overlay.resolve())]
started = time.monotonic()
before = digest(worker)
timed_out = False
try:
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=30)
    stdout, stderr, code = result.stdout, result.stderr, result.returncode
except subprocess.TimeoutExpired as exc:
    stdout, stderr, code = exc.stdout or b"", exc.stderr or b"", None
    timed_out = True
output.mkdir(parents=True, exist_ok=True)
(output / "stdout.txt").write_bytes(stdout)
(output / "stderr.txt").write_bytes(stderr)
errors = []
verification = output / "verification.json"
record = json.loads(verification.read_text(encoding="utf-8")) if verification.is_file() else None
if not record:
    errors.append("Missing worker report")
else:
    if [case["name"] for case in record["cases"]] != expected:
        errors.append("Case order/count differs")
    if record["helperSha256"] != before or digest(worker) != before:
        errors.append("Worker identity differs")
    if record["mode"] != args.mode or record["fullNativeIntegrationAccepted"] is not False:
        errors.append("Mode/scope differs")
    if args.mode == "fixed":
        if code != 0 or not record["passed"] or record["failedCases"] or record["messages"]:
            errors.append("Corrected acceptance did not pass cleanly")
        if not all(case["passed"] and not case["error"] and not case["messages"] for case in record["cases"]):
            errors.append("Corrected case failure")
    else:
        if code != 1 or record["passed"] or not record["negativeBaselineReproduced"]:
            errors.append("Expected negative baseline was not reproduced")
        if record["failedCases"] != expected or len(record["messages"]) != 23:
            errors.append("Negative baseline shape differs")
if stderr:
    errors.append("Unexpected standard error")
if timed_out:
    errors.append("Owned worker exceeded 30 seconds and was killed")
audit = dict(acceptedExpectedResult=not errors, mode=args.mode, childExitCode=code,
    timedOut=timed_out, elapsedSeconds=time.monotonic()-started, deadlineSeconds=30,
    workerSha256=before, launcherSha256=digest(Path(__file__)), caseNames=expected,
    errors=errors, outputSha256={name:digest(output/name) for name in
        ["stdout.txt", "stderr.txt", "verification.json", "messages.txt"] if (output/name).is_file()},
    scope="Supplied offscreen Qt fixtures only; baseline remains failed acceptance with child exit 1.",
    nativeOrDesktopAccepted=False)
(output / "launcher-audit.json").write_text(json.dumps(audit, indent=2)+"\n", encoding="utf-8", newline="\n")
print(json.dumps(dict(acceptedExpectedResult=not errors, childExitCode=code,
    timedOut=timed_out, caseCount=len(expected), errors=errors), indent=2))
raise SystemExit(0 if not errors else 1)
