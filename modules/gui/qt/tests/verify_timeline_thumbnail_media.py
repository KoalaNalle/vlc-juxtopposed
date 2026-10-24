# SPDX-License-Identifier: GPL-2.0-or-later
"""Verify saved native codec and selected ordinary preview evidence.

Does not run VLC or accept every codec, track-selection UI, power, latency,
blur, physical DPI, final packaging or the complete beta roadmap.
"""
from pathlib import Path
from hashlib import sha256
import argparse
import gzip
import json
import re

repo = Path(__file__).resolve().parents[4]
base = repo / "doc/juxtopposed-references/native/home-timeline-thumbnails-20261021"
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--source-root", type=Path, default=repo)
args = parser.parse_args()

def read(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))

def digest(path):
    return sha256(path.read_bytes()).hexdigest()

def verify(directory, records):
    directory = directory.resolve()
    for name, expected in records.items():
        path = (directory / name).resolve()
        assert path.is_relative_to(directory), name
        assert digest(path) == expected, name

native_dir = base / "formats-r53"
ordinary_dir = base / "ordinary-formats-r54"
native_receipt = read(native_dir / "checkpoint.json")
ordinary_receipt = read(ordinary_dir / "checkpoint.json")
for directory, receipt in [(native_dir, native_receipt), (ordinary_dir, ordinary_receipt)]:
    verify(args.source_root, receipt["sourceBindings"])
    verify(directory, receipt["artifacts"])
    for gate in ["resourceCostAccepted", "frameWaitWorkaroundPromoted", "responsivenessAccepted",
                 "backdropBlurAccepted", "physicalDpiAccepted", "fullFeatureAccepted", "fullBetaAccepted"]:
        assert receipt[gate] is False, gate
assert native_receipt["sourceBindings"] == ordinary_receipt["sourceBindings"]

native = read(native_dir / "verification.json")
assert native["passed"] and native["normalExit"] and native["exitCode"] == 0
assert native["sourceInputsUnchanged"] and native["identitiesUnchanged"] and not native["forcedCleanupUsed"]
assert native["counts"] == dict(tests=26, failures=0, errors=0, skipped=0)
assert native["sourceInputsSha256"] == digest(native_dir / "probes/inputs-formats-r53.json")
assert native["runnerSha256"] == digest(native_dir / "probes/run-formats-r53.py")
fixtures = read(native_dir / "fixtures/manifest.json")
assert fixtures["synthetic"] and len(fixtures["cases"]) == 7
assert set(native_receipt["cases"]) == set(fixtures["cases"])
picture_checks = 0
for name, case in fixtures["cases"].items():
    assert native["formatFixtureSha256"][name] == case["sha256"]
    report = read(native_dir / native_receipt["cases"][name])
    assert report["primaryTimeUnchanged"] and report["primaryResumedAndStopped"]
    if name == "audio-only.flac":
        assert len(report["requests"]) == 1 and report["requests"][0]["noVideoFallback"]
    else:
        assert [row["second"] for row in report["requests"]] == [2, 8, 14]
        assert all(row["referencePixelsMatch"] for row in report["requests"])
        for reference in case["references"].values():
            assert digest(native_dir / "fixtures" / reference["path"]) == reference["sha256"]
        picture_checks += len(report["requests"])
assert picture_checks == 18
tracks = fixtures["cases"]["hevc-multitrack.mkv"]["codecs"]
assert sum(row["type"] == "audio" for row in tracks) == 2
assert sum(row["type"] == "subtitle" for row in tracks) == 1

warning_review = read(ordinary_dir / "native-warning-review.json")
assert warning_review["allNativeWarningsAbsent"] is False
for name, path in ordinary_receipt["cases"].items():
    result = read(ordinary_dir / path)
    assert result["passed"] and result["normalExit"] and result["exitCode"] == 0
    assert result["sourceInputsUnchanged"] and result["identitiesUnchanged"] and not result["forcedCleanupUsed"]
    assert result["thumbnailJourneyHooksAbsent"] and not result["privateFixture"] and result["frameLatencyEnvironment"] is None
    assert result["sourceInputsSha256"] == digest(ordinary_dir / "probes/inputs-ordinary-formats-r54.json")
    assert result["runnerSha256"] == digest(ordinary_dir / "probes/run-ordinary-formats-r54.py")
    assert result["identities"] == ordinary_receipt["identities"]
    for item in ["libvlccore.dll", "plugins/gui/libqt_plugin.dll", "plugins/demux/libmp4_plugin.dll"]:
        assert result["identities"][item] == native["identities"][item]
    assert not result["diagnostics"] and result["realD3D11Opened"]
    assert any("Using D3D11VA (NVIDIA" in line for line in result["hardwareDecoderEvidence"])
    assert result["previewInspection"]["picturePreviewObserved"]
    assert result["finalInspection"]["independentPictureObserved"] and result["finalInspection"]["continuedPlaybackObserved"]
    order = result["quitLogOrder"]
    assert order["wait"] < order["stopped"] < order["teardown"]
    log = gzip.decompress((ordinary_dir / name / "vlc.log.gz").read_bytes()).decode("utf-8", errors="replace")
    assert [line for line in log.splitlines() if " warning:" in line] == warning_review["nativeWarnings"][name]
    assert not re.search(r"\berror:|qt (?:warning|critical|fatal):|Assertion failed", log)
assert ordinary_receipt["ordinarySelectedCodecUiAccepted"] and not ordinary_receipt["allCodecUiAccepted"]
print(f"Verified {len(native_receipt['sourceBindings'])} source and "
      f"{len(native_receipt['artifacts']) + len(ordinary_receipt['artifacts'])} artifact bindings: "
      "native26/0skips,18 codec pictures, audio fallback, two selected ordinary hardware-decoded preview/resume/Quit cases. Broader gates remain open.")
