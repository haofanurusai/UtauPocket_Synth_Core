"""One external wav2llsm conversion and its persistent analysis inputs."""
from i18n import get
from pathlib import Path
import subprocess
import wave
import numpy as np
from audio_utils import slice_wav
from project import RECORD


def convert_one(exe, source, output, params, task):
    alias, bank, program, oto = task
    symbol = ord(bank) | ord(program) << 8
    stem = f'{symbol:04x}'
    cache = Path(output) / '.utau-editor'
    cache.mkdir(exist_ok=True)
    cropped = cache / (stem + '.wav')
    source_wav = Path(source) / oto['wav_file']
    with wave.open(str(source_wav), 'rb') as stream:
        rate, count = stream.getframerate(), stream.getnframes()
    length = count / rate * 1000
    offset, overlap, cutoff = oto['offset'], oto['overlap'], oto['cutoff']
    left = min(offset, offset + overlap)
    right = length - cutoff if cutoff > 0 else offset - cutoff if cutoff < 0 else length
    fs = slice_wav(source_wav, cropped, left, right)
    args = [str(exe), str(cropped)]
    for key, option in [('bias','b'), ('har_e','e'), ('fmin','f'), ('fmax','F'),
                        ('hop','h'), ('har','H'), ('fft','n'), ('psd','p'),
                        ('trange','t'), ('spec','s'), ('bap','B')]:
        args += ['-' + option, str(params[key])]
    ls2 = bank + program + '.LS2'
    args += ['-o', str(Path(output) / ls2)]
    result = subprocess.run(args, check=True, capture_output=True, text=True,
                            creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
    frames = np.asarray([[float(v) for v in line.split(',')]
                         for line in result.stdout.splitlines()], np.float32)
    if (frames.ndim != 2 or frames.shape[1] != 2 or
            not 2 <= len(frames) <= 65535 or not np.isfinite(frames).all()):
        raise ValueError(get('expected_one_finite_f0_rd_pair_per_frame'))
    f0, rd = frames[:, 0], frames[:, 1]
    n = len(frames)
    hop = int(params['hop'])
    def frame(ms):
        return int(ms * fs / 1000) // hop
    shift = min(0, overlap)
    ovl, pre, con = frame(overlap-shift), frame(oto['preutterance']-shift), frame(oto['consonant']-shift)
    if not (0 <= ovl <= n and 0 <= pre <= n and 0 <= con <= n-2):
        raise ValueError(get('invalid_oto_timing_no_legal_loop_remains'))
    proposed = (int(n * params['loop_start_pct'] / 100), int(n * params['loop_end_pct'] / 100))
    loop_l = max(con, min(proposed[0], n-2))
    loop_r = max(loop_l+1, min(proposed[1], n-1))
    f0path = cache / (stem + '.f0.npy')
    np.save(f0path, f0, allow_pickle=False)
    rdpath = cache / (stem + '.rd.npy')
    np.save(rdpath, rd, allow_pickle=False)
    record = RECORD.pack(symbol, ovl, pre, con, loop_l, loop_r, n, int(overlap < 0))
    metadata = dict(symbol=symbol, alias=alias, ls2=ls2, fs=fs, hop=hop,
                    wav=str(cropped.relative_to(output)), f0=str(f0path.relative_to(output)),
                    rd=str(rdpath.relative_to(output)),
                    source=str(source_wav.resolve()),
                    crop_start_sample=max(0, int(left*fs/1000)),
                    crop_end_sample=min(count, int(right*fs/1000)))
    if proposed != (loop_l, loop_r):
        metadata['initial_adjustment'] = dict(requested=proposed, applied=(loop_l, loop_r))
    return record, metadata
