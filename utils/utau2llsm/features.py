"""Lazy ContentVec loading and explicit CPU/CUDA fallback."""
from i18n import get
import gc
import numpy as np


def align_features(features, source_times, nframes, hop, fs):
    features = np.asarray(features, np.float32)
    if features.ndim != 2 or features.shape[1] != 768 or not np.isfinite(features).all():
        raise ValueError(get('contentvec_must_return_finite_frames_768_features'))
    times = np.arange(nframes) * hop / fs
    return np.stack([np.interp(times, source_times, features[:, column])
                     for column in range(768)], axis=1).astype(np.float32)


class ContentVec:
    def __init__(self, path, try_gpu=False, status=lambda message: None):
        self.path, self.status = str(path), status
        self.model = None
        self.device = 'cuda' if try_gpu else 'cpu'
        try:
            self._load()
        except Exception as error:
            if self.device == 'cpu':
                raise
            message = str(error)
        else:
            return
        self._fallback(message)

    def _load(self):
        import torch
        from functools import partial
        from unittest.mock import patch
        from fairseq import checkpoint_utils

        self.torch = torch
        loader = partial(torch.load, weights_only=False)
        with patch.object(checkpoint_utils.torch, 'load', loader):
            models, cfg, _ = checkpoint_utils.load_model_ensemble_and_task(
                [self.path]
            )
        self.model = models[0].eval().to(self.device)
        self.normalize = bool(getattr(getattr(cfg, 'task', None), 'normalize', False))
        # Read the actual convolution kernels/strides instead of stretching by length.
        stride, receptive = 1, 1
        for layer in self.model.feature_extractor.conv_layers:
            conv = layer[0]
            if conv.padding[0] != 0 or conv.dilation[0] != 1:
                raise ValueError(get('unsupported_contentvec_convolution_geometry'))
            receptive += (conv.kernel_size[0]-1)*stride
            stride *= conv.stride[0]
        self.stride, self.receptive = stride, receptive
        self.status('ContentVec: ' + self.device.upper())

    def _fallback(self, error):
        self.status(get('cuda_failed_falling_back_to_cpu') + str(error))
        self.model = None
        gc.collect()
        if hasattr(self, 'torch') and self.torch.cuda.is_available():
            self.torch.cuda.empty_cache()
        self.device = 'cpu'
        self._load()

    def analyze(self, audio, fs, nframes, hop):
        try:
            return self._infer(audio, fs, nframes, hop)
        except Exception as error:
            if self.device == 'cpu':
                raise
            # Clear traceback references to failed CUDA tensors before reloading.
            message = str(error)
        self._fallback(message)
        return self._infer(audio, fs, nframes, hop)

    def _infer(self, audio, fs, nframes, hop):
        import torchaudio.functional as audio_ops
        torch = self.torch
        wave = torch.from_numpy(np.asarray(audio, np.float32)).unsqueeze(0)
        if fs != 16000:
            wave = audio_ops.resample(wave, fs, 16000)
        if wave.shape[-1] < self.receptive:
            deficit = self.receptive - wave.shape[-1]
            left = deficit//2
            wave = torch.nn.functional.pad(wave, (left, deficit-left))
        else:
            left = 0
        if self.normalize:
            wave = torch.nn.functional.layer_norm(wave, wave.shape)
        with torch.inference_mode():
            result, _ = self.model.extract_features(source=wave.to(self.device), padding_mask=None)
            result = result.squeeze(0).detach().cpu().numpy()
        times = (np.arange(len(result))*self.stride + (self.receptive-1)/2-left) / 16000
        return align_features(result, times, nframes, hop, fs)
