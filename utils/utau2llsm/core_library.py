"""Platform-specific shared core discovery and actionable loader diagnostics."""
import ctypes as ct
import os
from pathlib import Path
import struct
import sys
from i18n import get


def pe_machine(path, dll=False):
    with open(path, 'rb') as stream:
        header = stream.read(64)
        if len(header) != 64 or header[:2] != b'MZ':
            raise ValueError(get('not_a_windows_pe_file') + str(path))
        stream.seek(struct.unpack_from('<I', header, 60)[0])
        header = stream.read(24)
    if len(header) != 24 or header[:4] != b'PE\0\0':
        raise ValueError(get('invalid_pe_file') + str(path))
    if dll and not struct.unpack_from('<H', header, 22)[0] & 0x2000:
        raise ValueError(get('select_a_dll_not_an_exe_or_static_library') + str(path))
    return struct.unpack_from('<H', header, 4)[0]


def validate_windows_library(path, machine):
    actual = pe_machine(path, dll=True)
    if actual != machine:
        names = {0x14c:'Win32 (x86)', 0x8664:'x64', 0xaa64:'ARM64'}
        raise ValueError(get('dll_python_architecture_mismatch') +
                         f'{path}\nDLL: {names.get(actual,hex(actual))}; Python: {names.get(machine,hex(machine))}')


def find_library(precision='float'):
    override = os.environ.get('UTAU_POCKET_LIBRARY')
    if override:
        return override
    root = Path(__file__).resolve().parents[2]
    if os.name == 'nt':
        machine = pe_machine(sys.executable)
        platform = {0x14c:'Win32', 0x8664:'x64', 0xaa64:'ARM64'}.get(machine)
        name = 'utau_pocket.dll'
        candidates = [root/'build'/'msvc'/platform/config/precision/name
                      for config in ('ReleaseDLL','DebugDLL')] if platform else []
        candidates.append(Path(__file__).parent/name)
        for path in candidates:
            if path.is_file():
                try:
                    validate_windows_library(path,machine)
                except ValueError:
                    continue
                return str(path)
    else:
        name = 'libutau_pocket.dylib' if sys.platform == 'darwin' else 'libutau_pocket.so'
        candidates = [root/'build'/precision/'lib'/name, Path(__file__).parent/name]
        if sys.platform.startswith('linux'):
            candidates.append(root/'dist'/'linux-x86_64'/precision/name)
        for path in candidates:
            if path.is_file():
                return str(path)
    raise FileNotFoundError(get('no_matching_shared_core_build_or_select_one') +
                            '\n' + '\n'.join(map(str,candidates)))


def load_library(path, precision):
    path = Path(path or find_library(precision)).resolve()
    if os.name == 'nt':
        validate_windows_library(path, pe_machine(sys.executable))
    try:
        library = ct.CDLL(str(path))
        for name in ('init','close','synth','push_midi_evt'):
            getattr(library,'utau_pocket_'+name)
        return library
    except (OSError, AttributeError) as error:
        raise RuntimeError(get('cannot_load_core_check_core_exports_and_dependency_python_architecture') +
                           f'\n{path}\n{error}') from error
