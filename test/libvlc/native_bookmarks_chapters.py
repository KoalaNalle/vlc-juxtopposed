"""Run real production bookmark/chapter models with finite owned native execution."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess


CASES = ["initTestCase", "actualThreeChapterMetadataAndSelection",
         "actualBookmarkAddRenameSeekAndRemove",
         "actualMediaSwitchRejectsOldBookmarksBeforeGuiDispatch",
         "actualLateNativeMediaRegistrationRestoresAdd",
         "actualStopSameMediaRestartRetainsBookmark",
         "actualNativeRestartPersistsBookmarkAndMetadata",
         "actualModelDetachAndOwnerDestruction", "cleanupTestCase"]


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", type=Path, required=True)
    parser.add_argument("--test-exe", type=Path, required=True)
    parser.add_argument("--fixtures", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    package, executable, fixtures, output = [path.resolve() for path in
        (args.package, args.test_exe, args.fixtures, args.output_dir)]
    output.mkdir(parents=True, exist_ok=False)
    report = {"startedUtc": datetime.now(timezone.utc).isoformat(), "passed": False,
              "expectedCases": CASES, "package": str(package), "testExe": str(executable),
              "fixtures": str(fixtures), "timeoutSeconds": 180,
              "scope": "Real local ML indexing and production PlayerController, ChapterListModel and MLBookmarkModel on an owned native NORMAL-lock playlist. Real bookmark add/rename/seek/remove, media-switch rejection before GUI dispatch, native external registration, stop/restart/database restart and settled detach/destruction. No native Windows QML panel, visual, queued-in-flight worker teardown or supported-format claim."}

    def save():
        (output / "verification.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")

    try:
        required = [executable, package / "libvlc.dll", package / "libvlccore.dll",
                    package / "vlc-preparser.exe", package / "plugins/misc/libmedialibrary_plugin.dll",
                    fixtures / "chaptered-colors.mkv", fixtures / "paused-seek-colors.mp4"]
        for path in required:
            if not path.is_file():
                raise RuntimeError("Missing actual runtime or generated fixture: " + str(path))
        for name in ["libvlc.dll", "libvlccore.dll"]:
            adjacent = executable.parent / name
            if adjacent.exists():
                required.append(adjacent)
                if digest(adjacent) != digest(package / name):
                    raise RuntimeError("Adjacent and selected runtime hashes differ: " + name)
        report["inputHashes"] = {str(path): digest(path) for path in required}
        userdata = output / "userdata"
        userdata.mkdir()
        environment = os.environ.copy()
        environment.pop("VLC_PLUGIN_PATH", None)
        environment.update(PATH=str(package) + os.pathsep + environment.get("PATH", ""),
            VLC_LIB_PATH=str(package / "plugins"), VLC_LIBEXEC_PATH=str(package / "vlc-preparser.exe"),
            VLC_USERDATA_PATH=str(userdata), VLC_CHAPTER_FIXTURE_DIR=str(fixtures),
            QT_QPA_PLATFORM="offscreen", QTEST_FUNCTION_TIMEOUT="90000")
        report["arguments"] = ["-o", "results.txt,txt"]
        with (output / "stdout.txt").open("wb") as stdout, (output / "stderr.txt").open("wb") as stderr:
            process = subprocess.Popen([str(executable), "-o", "results.txt,txt"], cwd=output,
                env=environment, stdout=stdout, stderr=stderr,
                creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0)
            report["ownedPid"] = process.pid
            save()
            try:
                report["exitCode"] = process.wait(timeout=180)
            except subprocess.TimeoutExpired:
                report["timedOut"] = True
                process.kill()  # This finite runner owns only this subprocess.
                report["exitCode"] = process.wait(timeout=10)
        result = output / "results.txt"
        content = result.read_text(encoding="utf-8-sig", errors="replace") if result.exists() else ""
        passes = re.findall(r"^PASS\s*:\s*TestNativeBookmarksChapters::([^\(]+)\(\)", content, re.MULTILINE)
        totals = re.search(r"Totals:\s*(\d+) passed, (\d+) failed, (\d+) skipped", content)
        report["actualCases"] = passes
        report["totals"] = dict(zip(["passed", "failed", "skipped"], map(int, totals.groups()))) if totals else None
        logs = sorted(output.glob("bookmarks-chapters-native*.log"))
        expected_logs = ["bookmarks-chapters-native-1.log", "bookmarks-chapters-native-2.log"]
        log_texts = {path.name: path.read_text(encoding="utf-8-sig", errors="replace") for path in logs}
        captured = "\n".join(path.read_text(encoding="utf-8-sig", errors="replace")
            for path in [output / "stdout.txt", output / "stderr.txt", *logs])
        report["startupError"] = bool(re.search(r"unknown option|unrecognized option|Media library initialization failed|no suitable medialibrary module", captured, re.I))
        report["lanStartup"] = bool(re.search(r"mDNS:\s*listening|Initializing libupnp|(?:using|looking for) services_discovery module[^\r\n]*[\"'](?:microdns|sap|upnp|avahi|bonjour)[\"']|datagram port 9875|224\.2\.127\.254", captured, re.I))
        report["initializedInstances"] = {name:
            "using medialibrary module" in log_texts.get(name, "") and
            ("MediaLibrary successfully initialized" in log_texts.get(name, "") or
             "Database was reset" in log_texts.get(name, "")) for name in expected_logs}
        report["nativeLogsPresent"] = ([path.name for path in logs] == expected_logs)
        report["evidenceHashes"] = {str(path.relative_to(output)): digest(path)
            for path in [output / "stdout.txt", output / "stderr.txt", *logs, result] if path.is_file()}
        report["passed"] = (report["exitCode"] == 0 and passes == CASES
            and report["totals"] == {"passed": len(CASES), "failed": 0, "skipped": 0}
            and not re.search(r"^(?:FAIL!|SKIP)\s*:", content, re.MULTILINE)
            and not report["startupError"] and not report["lanStartup"]
            and report["nativeLogsPresent"] and all(report["initializedInstances"].values()))
    except Exception as error:
        report["error"] = str(error)
    report["finishedUtc"] = datetime.now(timezone.utc).isoformat()
    save()
    print(json.dumps({key: report.get(key) for key in
        ("passed", "exitCode", "totals", "timedOut", "error")}, indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
