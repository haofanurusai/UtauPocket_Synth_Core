"""PCM WAV cropping and display input; no optional ML dependencies."""
from i18n import get
import wave
import numpy as np


def slice_wav(source, destination, start_ms, end_ms):
    with wave.open(str(source), 'rb') as stream:
        rate = stream.getframerate()
        start = max(0, min(stream.getnframes(), int(start_ms * rate / 1000)))
        end = stream.getnframes() if end_ms is None else min(
            stream.getnframes(), int(end_ms * rate / 1000))
        if end <= start:
            raise ValueError(get('empty_wav_crop'))
        stream.setpos(start)
        data, params = stream.readframes(end - start), stream.getparams()
    with wave.open(str(destination), 'wb') as stream:
        stream.setparams(params)
        stream.writeframes(data)
    return rate


def read_wav(path):
    with wave.open(str(path), 'rb') as stream:
        width, channels, rate = stream.getsampwidth(), stream.getnchannels(), stream.getframerate()
        data = stream.readframes(stream.getnframes())
    if width == 1:
        samples = (np.frombuffer(data, np.uint8).astype(np.float32) - 128) / 128
    elif width in (2, 4):
        samples = np.frombuffer(data, '<i' + str(width)).astype(np.float32) / (2 ** (8 * width - 1))
    elif width == 3:
        values = np.frombuffer(data, np.uint8).reshape(-1, 3).astype(np.int32)
        samples = values[:, 0] | values[:, 1] << 8 | values[:, 2] << 16
        samples = ((samples ^ 0x800000) - 0x800000).astype(np.float32) / 8388608
    else:
        raise ValueError(get('unsupported_pcm_sample_width'))
    return samples.reshape(-1, channels).mean(axis=1), rate
