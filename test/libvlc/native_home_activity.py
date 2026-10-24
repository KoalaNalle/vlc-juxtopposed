"""Run local ML diagnostics with real discovery and an explicit no-LAN audit."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time


FIXTURE_NAMES = ["alpha-one.flac", "alpha-two.flac", "beta-one.flac",
                 "beta-two.flac", "home-video.mp4"]
LAN_DLL_NAMES = ["libmicrodns_plugin.dll", "libsap_plugin.dll", "libupnp_plugin.dll"]
SUITES = {
    "home-activity": {
        "class": "TestHomeActivity", "fixtureEnv": "VLC_HOME_FIXTURE_DIR",
        "logPrefix": "home-activity-native", "instances": 3,
        "cases": ["initTestCase", "persistentVisitTypesAndLifetime", "malformedStoredVisits",
                  "preferencesResetDoesNotRestoreCachedVisits", "realFolderPlaylistAndArtistMetadata",
                  "warmRestartAndDeletedPlaylist", "blockedStoragePreservesAcceptedVisitsAndDocument",
                  "blockedClearPreservesAcceptedVisitsAndDocument", "addedFolderRequestStateAndReset",
                  "actualAudioOnlyAddedFolderAndMixedRouteRestart", "cleanupTestCase"],
        "scope": "Production Home store, snapshot and request helpers with real local ML indexing/events/restart; no full MLHomeActivity runner, folder picker, QML or desktop claim.",
    },
    "new-playlist": {
        "class": "TestNewPlaylist", "fixtureEnv": "VLC_PLAYLIST_FIXTURE_DIR",
        "logPrefix": "new-playlist-native", "instances": 2,
        "cases": ["initTestCase", "validationAndSelectionOrder", "rollbackAndPartialFailure",
                  "actualMixedSaveWithoutPlayback", "actualExternalMediaUnknownDurationWithoutDiscovery",
                  "actualThumbnailWriteFailureRollsBack", "actualEmptyPlaylist", "persistenceAfterRestart",
                  "submittedSaveCompletesBeforeModelDestruction", "cleanupTestCase"],
        "scope": "Actual native local discovery, ordered duplicate mixed save, truthful external UNKNOWN duration, copied PNG, restart and owner shutdown; no desktop GUI claim.",
    },
}
LAN_PATTERNS = [
    r'(?:looking for|using) services_discovery module[^\r\n]*["\'](?:microdns|sap|upnp|avahi|bonjour)["\']',
    r'mDNS:\s*listening',
    r'Initializing libupnp',
    r'net:\s*opening[^\r\n]*(?:datagram port 9875|224\.2\.127\.254)',
]
STARTUP_ERROR_PATTERNS = [
    r'unknown option', r'unrecognized option', r'Media library initialization failed',
    r'Medialibrary failed to initialize', r'no suitable medialibrary module',
]
MARKER = re.compile(r'^VLC_LOCAL_ML_DIAGNOSTIC serial=(\d+) '
                    r'option=--no-ml-network-discovery logfile=(\S+)$', re.MULTILINE)


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def audit_logs(output, suite):
    """Require each opt-in/restart log, then reject actual LAN startup paths."""
    expected = [f'{suite["logPrefix"]}-{serial}.log'
                for serial in range(1, suite["instances"] + 1)]
    observed = sorted(path.name for path in output.glob(suite["logPrefix"] + "-*.log"))
    inputs = [output / "stdout.txt", output / "stderr.txt"] + [output / name for name in expected]
    missing = [path.name for path in inputs if not path.is_file()]
    texts = {path.name: path.read_text(encoding="utf-8-sig", errors="replace")
             for path in inputs if path.is_file()}
    markers = [(int(serial), filename) for serial, filename in
               MARKER.findall(texts.get("stderr.txt", ""))]
    expected_markers = list(enumerate(expected, 1))
    hits = []
    errors = []
    for filename, content in texts.items():
        for pattern in LAN_PATTERNS:
            hits.extend({"file": filename, "match": found.group(0)}
                        for found in re.finditer(pattern, content, re.IGNORECASE))
        for pattern in STARTUP_ERROR_PATTERNS:
            errors.extend({"file": filename, "match": found.group(0)}
                          for found in re.finditer(pattern, content, re.IGNORECASE))
    # Each instance must both load the plugin and initialize a real native ML.
    initialized = {
        name: ("using medialibrary module" in texts.get(name, "")
               and ("MediaLibrary successfully initialized" in texts.get(name, "")
                    or "Database was reset" in texts.get(name, "")))
        for name in expected
    }
    passed = (not missing and observed == expected and markers == expected_markers
              and all(initialized.values()) and not hits and not errors)
    return {"passed": passed, "expectedInstanceLogs": expected,
            "observedInstanceLogs": observed, "missingLogs": missing,
            "optInMarkers": markers, "initializedInstances": initialized,
            "lanStartupMatches": hits, "startupErrors": errors,
            "checkedLogs": list(texts),
            "limit": "Runtime logs cover all diagnostic instances and captured helper stdio; quiet external preparser compatibility also relies on its source contract (no ML enabled). This does not assert that arbitrary VLC network playback is prohibited."}


def audit_results(content, suite):
    passes = re.findall(r'^PASS\s*:\s*' + re.escape(suite["class"]) + r'::([^\(]+)\(\)',
                        content, re.MULTILINE)
    total = re.search(r'Totals:\s*(\d+) passed, (\d+) failed, (\d+) skipped', content)
    totals = dict(zip(["passed", "failed", "skipped"], map(int, total.groups()))) if total else None
    accepted = (passes == suite["cases"]
                and totals == {"passed": len(suite["cases"]), "failed": 0, "skipped": 0}
                and not re.search(r'^(?:FAIL!|SKIP)\s*:', content, re.MULTILINE))
    return {"passed": accepted, "actualPasses": passes, "totals": totals}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", type=Path, required=True)
    parser.add_argument("--test-exe", type=Path, required=True)
    parser.add_argument("--fixtures", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--suite", choices=SUITES, default="home-activity")
    args = parser.parse_args()
    package, executable, fixtures, output = (path.resolve() for path in
        [args.package, args.test_exe, args.fixtures, args.output_dir])
    suite = SUITES[args.suite]
    output.mkdir(parents=True, exist_ok=False)
    report = {"startedUtc": datetime.now(timezone.utc).isoformat(), "suite": args.suite,
              "scope": suite["scope"], "package": str(package), "testExe": str(executable),
              "fixtures": str(fixtures), "isolatedUserdata": True,
              "localMlOptIn": "VLC_TEST_LOCAL_ML=1", "expectedPasses": suite["cases"],
              "processTimeoutSeconds": 150, "passed": False, "timedOut": False}

    def save():
        (output / "verification.json").write_text(
            json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")

    try:
        lan_plugins = [package / "plugins" / "services_discovery" / name for name in LAN_DLL_NAMES]
        ml_plugin = package / "plugins" / "misc" / "libmedialibrary_plugin.dll"
        required = [executable, package / "libvlc.dll", package / "libvlccore.dll",
                    package / "vlc-preparser.exe", ml_plugin, *lan_plugins]
        required += [fixtures / name for name in FIXTURE_NAMES]
        for path in required:
            if not path.is_file():
                raise RuntimeError("Missing required native runtime, ordinary LAN provider or fixture: " + str(path))
        for name in ["libvlc.dll", "libvlccore.dll"]:
            adjacent = executable.parent / name
            if adjacent.exists():
                required.append(adjacent)
                if digest(adjacent) != digest(package / name):
                    raise RuntimeError("Adjacent and package native DLL hashes differ: " + name)
        report["nativeCoreHashesMatch"] = True
        report["inputHashes"] = {str(path): digest(path) for path in required}
        report["ordinaryLanPluginsPreserved"] = {path.name: digest(path) for path in lan_plugins}
        userdata = output / "userdata"
        userdata.mkdir()
        environment = os.environ.copy()
        # Windows config_GetSysPath honors VLC_LIB_PATH verbatim. A supplementary
        # VLC_PLUGIN_PATH would add another inventory; use only the full package.
        environment.pop("VLC_PLUGIN_PATH", None)
        environment.update(PATH=str(package) + os.pathsep + environment.get("PATH", ""),
            VLC_LIB_PATH=str(package / "plugins"), VLC_LIBEXEC_PATH=str(package / "vlc-preparser.exe"),
            VLC_USERDATA_PATH=str(userdata), VLC_TEST_LOCAL_ML="1", QT_QPA_PLATFORM="offscreen")
        environment[suite["fixtureEnv"]] = str(fixtures)
        report["diagnosticEnvironment"] = {name: environment[name] for name in
            ["VLC_LIB_PATH", "VLC_LIBEXEC_PATH", "VLC_USERDATA_PATH", "VLC_TEST_LOCAL_ML", suite["fixtureEnv"]]}
        report["arguments"] = ["-o", "results.txt,txt"]
        start = time.monotonic()
        hidden = getattr(subprocess, "CREATE_NO_WINDOW", 0)
        with (output / "stdout.txt").open("wb") as stdout, (output / "stderr.txt").open("wb") as stderr:
            process = subprocess.Popen([str(executable), *report["arguments"]], cwd=output,
                env=environment, stdout=stdout, stderr=stderr, creationflags=hidden)
            report["ownedProcessId"] = process.pid
            save()
            print("Owned local ML diagnostic:", process.pid, "output:", output, flush=True)
            try:
                report["exitCode"] = process.wait(timeout=150)
            except subprocess.TimeoutExpired:
                report["timedOut"] = True
                # Only this owned test tree, solely for the finite diagnostic deadline.
                if os.name == "nt":
                    terminated = subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"],
                        capture_output=True, timeout=10, creationflags=hidden)
                    report["ownedDeadlineTerminationExitCode"] = terminated.returncode
                if process.poll() is None:
                    process.kill()
                report["exitCode"] = process.wait(timeout=10)
        report["elapsedSeconds"] = round(time.monotonic() - start, 3)
        results = output / "results.txt"
        content = results.read_text(encoding="utf-8-sig", errors="replace") if results.is_file() else ""
        report["nativeCases"] = audit_results(content, suite)
        report["localOnlyLogAudit"] = audit_logs(output, suite)
        report["passed"] = (not report["timedOut"] and report["exitCode"] == 0
                            and report["nativeCases"]["passed"] and report["localOnlyLogAudit"]["passed"])
        print(content, flush=True)
    except Exception as error:
        report["runnerError"] = type(error).__name__ + ": " + str(error)
        print(report["runnerError"], flush=True)
    finally:
        report["outputHashes"] = {path.name: digest(path) for path in output.iterdir()
                                  if path.is_file() and path.name != "verification.json"}
        report["finishedUtc"] = datetime.now(timezone.utc).isoformat()
        save()
    print("Native local ML acceptance:", report["passed"], flush=True)
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
