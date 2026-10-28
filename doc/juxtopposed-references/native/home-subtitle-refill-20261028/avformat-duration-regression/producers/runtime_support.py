"""Read-only guards and fail-closed staging for reviewed observations."""
from hashlib import sha256
import os
from pathlib import Path
import shutil

def digest(path):
    result = sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            result.update(chunk)
    return result.hexdigest()

def safe_digest(path, errors):
    try:
        return digest(path)
    except BaseException as error:
        errors.append(f'{path}: {error!r}')
        return None

def manifest(root):
    root = Path(root)
    entries = {}
    for path in [root, *sorted(root.rglob('*'))]:
        state = path.lstat()
        assert not path.is_symlink() and not getattr(state, 'st_file_attributes', 0) & 1024, f'Reparse entry: {path}'
        if path.is_file():
            entries[path.relative_to(root).as_posix()] = {'sha256': digest(path), 'bytes': state.st_size}
    return entries

def manifest_matches(root, identities, errors):
    try:
        return manifest(root) == identities
    except BaseException as error:
        errors.append(f'runtime inventory {root}: {error!r}')
        return False

def storage_state(path, budget):
    disk = shutil.disk_usage(path)
    assert budget['reserveFraction'] == .2
    reserve = (disk.total + 4) // 5
    headroom = disk.free - reserve
    assert sum(budget['componentsBytes'].values()) == budget['peakAdditionalBytes']
    return {'totalBytes': disk.total, 'freeBytes': disk.free, 'minimumFreeBytes': reserve,
            'availableBefore80PercentBytes': headroom,
            'peakAdditionalBytes': budget['peakAdditionalBytes'],
            'fits': headroom >= budget['peakAdditionalBytes'],
            'remainingAfterWorstPeakBytes': headroom - budget['peakAdditionalBytes']}

def stage_hardlinks(source, destination, frozen):
    """Never fall back to copying2.83GB after an NTFS hardlink-limit error."""
    source, destination = Path(source), Path(destination)
    assert source.drive.upper() == destination.drive.upper()
    assert not destination.exists() and manifest(source) == frozen
    destination.mkdir()
    linked = 0
    for path in sorted(source.rglob('*')):
        relative = path.relative_to(source)
        if relative.parts[0] == 'portable' or path.name == 'plugins.dat':
            continue
        target = destination / relative
        if path.is_dir():
            target.mkdir(exist_ok=True)
        elif path.is_file():
            os.link(path, target)  # Any error, including EMLINK, aborts staging.
            linked += 1
    (destination / 'portable').mkdir()
    return {'hardLinkedFiles': linked, 'copiedLinkLimitFiles': 0,
            'policy': 'hardlink-only-fail-closed'}
