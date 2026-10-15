import sys
import os
from i18n import I18N
from gui import ConverterApp


def get_executable_paths():
    """
    Locate wav2llsm beside the script or in the default float build/distribution.
    An explicit UTAU_WAV2LLSM path can select another precision or build.
    """
    if getattr(sys, 'frozen', False):
        application_path = os.path.dirname(sys.executable)
    else:
        application_path = os.path.dirname(os.path.abspath(__file__))

    exe_ext = ".exe" if os.name == 'nt' else ""
    override = os.environ.get('UTAU_WAV2LLSM')
    if override:
        return {'wav2llsm': os.path.abspath(override)}
    root = os.path.abspath(os.path.join(application_path, '..', '..'))
    candidates = [
        os.path.join(application_path, f"wav2llsm{exe_ext}"),
        os.path.join(root, 'build', 'float', 'bin', f"wav2llsm{exe_ext}"),
        os.path.join(root, 'dist', 'linux-x86_64', 'float', f"wav2llsm{exe_ext}"),
    ]
    return {'wav2llsm': next((p for p in candidates if os.path.isfile(p)),
                             candidates[0])}


if __name__ == "__main__":
    i18n_manager = I18N(lang="zh_CN")
    exe_paths = get_executable_paths()

    app = ConverterApp(i18n_manager, exe_paths)
    app.mainloop()
