"""Hold only R25's generated INI against atomic replacement, then release it."""
import ctypes
from ctypes import wintypes
import hashlib
import json
from pathlib import Path
import os
import time

ROOT = Path(__file__).resolve().parents[2]
RUN = ROOT / 'build-windows/beta-resume-20261029/gui-r25'
TARGET = RUN / 'runtime/portable/vlc/vlc-qt-interface.ini'
OUT = RUN / 'preset-storage-lock-r1'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    assert not OUT.exists() and TARGET.is_file()
    assert TARGET.resolve().is_relative_to(RUN.resolve()) and not TARGET.is_symlink()
    record = {'target': str(TARGET), 'beforeSha256': sha(TARGET), 'helperSha256': sha(__file__),
              'pid': os.getpid(), 'status': 'starting', 'released': False}
    OUT.mkdir()

    def save():
        (OUT / 'verification.json').write_text(json.dumps(record, indent=2) + '\n')

    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.CreateFileW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD,
                                  wintypes.LPVOID, wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE]
    kernel.CreateFileW.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.CloseHandle.restype = wintypes.BOOL
    handle = kernel.CreateFileW(str(TARGET), 0xC0000000, 3, None, 3, 0x80, None)
    assert handle not in (None, ctypes.c_void_p(-1).value), ctypes.get_last_error()
    try:
        record.update(status='holding-generated-ini', handleOpened=True)
        save()
        print('OWNED_PRESET_INI_LOCK ' + str(os.getpid()), flush=True)
        deadline = time.monotonic() + 1200
        while not (OUT / 'release.txt').exists() and time.monotonic() < deadline:
            time.sleep(0.2)
        record.update(releaseRequested=(OUT / 'release.txt').exists(),
                      afterFailedSaveSha256=sha(TARGET))
        record['failedSaveKeptDiskBytes'] = record['afterFailedSaveSha256'] == record['beforeSha256']
    finally:
        assert kernel.CloseHandle(handle)
        record.update(status='terminal', released=True)
        save()
    print(json.dumps(record), flush=True)
    assert record['releaseRequested'] and record['failedSaveKeptDiskBytes']


if __name__ == '__main__':
    main()
