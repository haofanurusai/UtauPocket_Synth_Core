"""Background analysis; all database edits are returned to the GUI thread."""
import io
import hashlib
from pathlib import Path
import numpy as np
from audio_utils import read_wav
from features import ContentVec
from loop_analysis import find_loop
from project import atomic_write, digest


def cache_path(project, entry):
    return project.root / '.utau-editor' / f'{entry.symbol:04x}.features.npz'


def fingerprint(project, entry):
    return digest(project.asset(entry.meta['wav']).read_bytes())


def load_cached(project, entry):
    try:
        with np.load(cache_path(project, entry), allow_pickle=False) as cache:
            if (str(cache['audio_hash']) == fingerprint(project, entry)
                    and int(cache['hop']) == entry.meta['hop']
                    and int(cache['fs']) == entry.meta['fs']
                    and cache['features'].shape == (entry.fields[6], 768)):
                return cache['features'].copy()
    except (OSError, ValueError, KeyError):
        pass
    return None


def run_analysis(project, indices, model_path, gpu, auto, cancel, messages, params=None):
    edits, errors = {}, []
    try:
        analyzer = ContentVec(model_path, gpu, lambda s: messages.put(('status', s)))
        checksum = hashlib.sha256()
        with Path(model_path).open('rb') as model_file:
            for chunk in iter(lambda: model_file.read(1024*1024), b''):
                checksum.update(chunk)
        model_hash = checksum.hexdigest()
        for count, index in enumerate(indices, 1):
            if cancel.is_set():
                break
            entry = project.entries[index]
            try:
                audio, fs = read_wav(project.asset(entry.meta['wav']))
                f0 = np.load(project.asset(entry.meta['f0']), allow_pickle=False)
                features = analyzer.analyze(audio, fs, entry.fields[6], entry.meta['hop'])
                buffer = io.BytesIO()
                np.savez_compressed(buffer, features=features, audio_hash=fingerprint(project, entry),
                                    model_hash=model_hash, hop=entry.meta['hop'], fs=fs)
                atomic_write(cache_path(project, entry), buffer.getvalue())
                messages.put(('features', (index, features)))
                if auto and not cancel.is_set():
                    rd = np.load(project.asset(entry.meta['rd']), allow_pickle=False)
                    edits[index] = find_loop(audio, fs, entry.meta['hop'], f0, rd, features,
                                             entry.fields[3], cancel, params)
            except InterruptedError:
                break
            except Exception as error:
                errors.append(entry.alias + ': ' + str(error))
            messages.put(('status', f'ContentVec {count}/{len(indices)}'))
    except Exception as error:
        errors.append(str(error))
    finally:
        messages.put(('analysis_done', (edits, errors, cancel.is_set())))
