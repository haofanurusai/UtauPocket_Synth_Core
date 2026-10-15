"""Single-owner synthesis and blocking audio output, away from Tk."""
from queue import Queue, Empty
from threading import Thread
import wave
import numpy as np
from preview_core import PreviewCore
from i18n import get


class PreviewWorker:
    def __init__(self, messages):
        self.commands, self.messages = Queue(), messages
        self.core, self.stream, self.remaining, self.note = None, None, None, 60
        self.thread = Thread(target=self._run, daemon=True, name='utau-preview')
        self.thread.start()

    def send(self, command, *args):
        self.commands.put((command, args))

    def _stop(self):
        if self.stream is not None:
            try:
                self.stream.abort()
                self.stream.close()
            finally:
                self.stream = None
        if self.core is not None:
            self.core.close()
            self.core = None

    def _start(self, request):
        self._stop()
        project, entry, bounds, library, controls, note, velocity = request
        self.core = PreviewCore(library, project, entry, bounds)
        for cc, value in controls.items():
            self.core.midi(0xB0, int(cc), int(value))
        self.core.midi(0xB0, 32, entry.symbol & 127)
        self.core.midi(0xC0, entry.symbol >> 8)
        # Core maps bank_char to CC32, prog_char to Program Change.
        self.note = note
        self.core.midi(0x90, note, velocity)
        self.remaining = None

    def _export(self, request, path):
        self._start(request)
        try:
            with wave.open(str(path), 'wb') as output:
                output.setparams((1, 2, self.core.fs, 0, 'NONE', 'not compressed'))
                blocks = int(3*self.core.fs/self.core.block)
                release = int(2*self.core.fs/self.core.block)
                for i in range(blocks):
                    if i == release:
                        self.core.midi(0x80, self.note, 0)
                    audio = self.core.render()
                    output.writeframes((np.clip(audio, -1, 1)*32767).astype('<i2').tobytes())
            self.messages.put(('status', str(path)))
        finally:
            self._stop()

    def _run(self):
        while True:
            try:
                try:
                    command, args = self.commands.get(timeout=0 if self.stream else .1)
                except Empty:
                    command, args = None, ()
                if command == 'quit':
                    break
                if command == 'stop':
                    self._stop()
                elif command == 'play':
                    try:
                        import sounddevice as sd
                    except (ImportError, OSError) as error:
                        raise RuntimeError(get('cannot_load_audio_output_check_sounddevice_portaudio_installation_and_architecture') + str(error)) from error
                    self._start(args[0])
                    self.stream = sd.OutputStream(samplerate=self.core.fs, channels=1,
                                                  dtype='float32', blocksize=self.core.block)
                    self.stream.start()
                elif command == 'export':
                    self._export(*args)
                elif command == 'cc' and self.core:
                    self.core.midi(0xB0, *args)
                elif command == 'release' and self.core:
                    self.core.midi(0xB0, 123, 0)
                    self.remaining = int(self.core.fs * 12 / self.core.block)
                if self.stream:
                    self.stream.write(np.clip(self.core.render(), -1, 1).reshape(-1, 1))
                    if self.remaining is not None:
                        self.remaining -= 1
                        if self.remaining <= 0:
                            self._stop()
            except Exception as error:
                self._stop()
                self.messages.put(('preview_error', str(error)))
        self._stop()
        self.messages.put(('preview_closed', None))
