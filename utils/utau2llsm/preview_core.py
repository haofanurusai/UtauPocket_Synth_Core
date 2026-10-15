"""ctypes adapter; used exclusively by one preview worker thread."""
from i18n import get
import ctypes as ct
from pathlib import Path
import os
import shutil
import tempfile
import numpy as np
from project import RECORD


from core_library import find_library, load_library


class PreviewCore:
    def __init__(self, library, project, entry, bounds, block=512):
        self.temp, self.lib = None, None
        ls2 = project.asset(entry.meta['ls2'])
        with ls2.open('rb') as stream:
            magic = stream.read(4)
        if magic not in (b'LS2F', b'LS2D'):
            raise ValueError(get('unknown_ls2_precision'))
        self.precision = 'float' if magic == b'LS2F' else 'double'
        scalar = ct.c_float if self.precision == 'float' else ct.c_double
        self.block = block
        self.buffer = (scalar * block)()
        self.lib = load_library(library, self.precision)
        self.lib.utau_pocket_init.argtypes = [ct.c_wchar_p if os.name == 'nt' else ct.c_char_p, ct.c_int]
        self.lib.utau_pocket_init.restype = ct.c_int
        self.lib.utau_pocket_close.argtypes = []
        self.lib.utau_pocket_close.restype = None
        self.lib.utau_pocket_synth.argtypes = [ct.POINTER(scalar)]
        self.lib.utau_pocket_synth.restype = ct.c_int
        self.lib.utau_pocket_push_midi_evt.argtypes = [ct.POINTER(ct.c_uint8), ct.c_size_t]
        self.lib.utau_pocket_push_midi_evt.restype = ct.c_int
        try:
            self.temp = tempfile.TemporaryDirectory(prefix='preview-', dir=project.root/'.utau-editor')
            destination = Path(self.temp.name) / ls2.name
            try:
                os.link(ls2, destination)
            except OSError:
                shutil.copy2(ls2, destination)
            fields = entry.fields.copy()
            entry.validate(bounds)
            fields[4:6] = bounds
            database = Path(self.temp.name)/'oto.db'
            database.write_bytes(RECORD.pack(*fields))
            self.fs = self.lib.utau_pocket_init(str(database) if os.name == 'nt' else os.fsencode(database), block)
            if self.fs <= 0:
                raise RuntimeError(get('core_init_failed_check_float_double_precision')+str(self.fs))
        except Exception:
            self.close()
            raise

    def midi(self, *values):
        data = (ct.c_uint8 * len(values))(*values)
        status = self.lib.utau_pocket_push_midi_evt(data, len(values))
        if status < 0:
            raise RuntimeError(f'MIDI: {status}')

    def render(self):
        status = self.lib.utau_pocket_synth(self.buffer)
        if status != self.block:
            raise RuntimeError(get('synthesis_failed')+str(status))
        result = np.ctypeslib.as_array(self.buffer).astype(np.float32)
        if not np.isfinite(result).all():
            raise RuntimeError(get('non_finite_audio_from_core'))
        return result

    def close(self):
        if self.lib is not None:
            self.lib.utau_pocket_close()
            self.lib = None
        if self.temp is not None:
            self.temp.cleanup()
            self.temp = None
