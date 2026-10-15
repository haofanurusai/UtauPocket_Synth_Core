# UtauPocket Synth Core

TBC

## Third-party components and licensing inventory

| Component / local path | Upstream | Included licensing information | Status / remaining work |
| --- | --- | --- | --- |
| `lib/libllsm2` | [Sleepwalking/libllsm2](https://github.com/Sleepwalking/libllsm2) | `LICENSE`; source headers specify GPL-3.0-or-later | Header version 2.1.0; locally modified. License matches upstream master as checked on 15 Oct 2026. Record the exact imported commit and local changes. |
| `lib/arduino_midi_library` | [FortySevenEffects/arduino_midi_library](https://github.com/FortySevenEffects/arduino_midi_library) | `LICENSE` and MIT source headers | Header version 5.0.0; locally modified. License matches upstream master as checked on 15 Oct 2026. Record the imported commit and changes. |
| `lib/ciglet` | [Sleepwalking/ciglet](https://github.com/Sleepwalking/ciglet) | BSD-3-Clause text in `ciglet.c` and `ciglet.h` | Record the imported revision and changes; add a standalone copy of its original license. External files below have separate notices. |
| `lib/libpyin` | [Sleepwalking/libpyin](https://github.com/Sleepwalking/libpyin) | BSD-3-Clause source headers | Record the imported revision and changes; add a standalone copy of its original license. |
| `lib/libgvps` | [Sleepwalking/libgvps](https://github.com/Sleepwalking/libgvps) | BSD-3-Clause `LICENSE` | License matches upstream master as checked on 15 Oct 2026. Record the imported revision and changes. |
| `apps/player/vendor/rtmidi` | [thestk/rtmidi](https://github.com/thestk/rtmidi) | MIT-style `LICENSE`, including a non-binding request | Included version 6.0.0. License matches upstream master as checked on 15 Oct 2026; record the exact source revision. |
| `apps/player/vendor/portaudio` | [PortAudio/portaudio](https://github.com/PortAudio/portaudio) | MIT-style `LICENSE.txt` and per-file notices | Included source subset is documented as 19.7.0. License matches upstream master as checked on 15 Oct 2026; record the exact source revision and any changes. |
| `apps/player/vendor/tinyalsa` | [tinyalsa/tinyalsa](https://github.com/tinyalsa/tinyalsa) | `LICENSE` points to BSD-style `NOTICE`; source files carry additional copyright notices | `LICENSE` and `NOTICE` match upstream master as checked on 15 Oct 2026. Keep both files; record the imported revision and subset. |
| `apps/player/vendor/asiosdk` | [Steinberg ASIO SDK](https://www.steinberg.net/developers/asiosdk-open/) | Dual Steinberg proprietary / GPLv3 licenses in `LICENSE.txt` and `common/LICENSE.txt`; separate licensing and trademark documents | Both license files match the official 2.3.4 distribution dated 15 Oct 2025. The intended integration path is GPLv3. Preserve embedded notices and review separate asset/trademark terms. This is not a claim that 2.3.4 is the latest SDK release. |
| `lib/ciglet/external/fastapprox-all.h` | Paul Mineiro, [fastapprox](https://github.com/etheory/fastapprox) | Embedded BSD-style notices | Preserve all embedded notices and confirm the exact source revision. |
| `lib/ciglet/external/fftsg_h.c` | [Takuya Ooura FFT](https://www.kurims.kyoto-u.ac.jp/~ooura/fft.html) | Embedded permission to use, copy, modify and distribute, with a request to reference the package when modified | Preserve the original notice; identify the imported release and changes. |
| `lib/ciglet/external/fast_median.c` | Nicolas Devillard / algorithms credited in the file | Embedded permissive statement and routine-specific provenance comments | Retain the complete comments; verify provenance before assigning a standardized license identifier. |
| `lib/ciglet/external/wavfile.c` | Masanori Morise / [WORLD](https://github.com/mmorise/World), with Kanru Hua modifications | Copyright and modification history; no complete standalone grant in this file | Identify the original WORLD revision and include the applicable license. Do not infer authorization solely from the copyright header. |
| `include/optparse.h` | [skeeto/optparse](https://github.com/skeeto/optparse) | Embedded public-domain dedication | Preserve the notice and confirm the imported revision. |
