"""Project model, lossless database editing, undo and atomic persistence."""
from i18n import get
from dataclasses import dataclass
from pathlib import Path
import hashlib
import json
import os
import struct
import tempfile

RECORD = struct.Struct('<8H')


def atomic_write(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix=path.name + '.', dir=path.parent)
    try:
        with os.fdopen(fd, 'wb') as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def write_json(path, value):
    atomic_write(path, json.dumps(value, ensure_ascii=False, indent=2).encode('utf8'))


def digest(data):
    return hashlib.sha256(data).hexdigest()


@dataclass
class Entry:
    fields: list
    meta: dict
    saved: tuple

    @property
    def bounds(self):
        return tuple(self.fields[4:6])

    @property
    def dirty(self):
        return self.bounds != self.saved

    @property
    def symbol(self):
        return self.fields[0]

    @property
    def alias(self):
        return self.meta.get('alias', str(self.symbol))

    def validate(self, bounds):
        left, right = bounds
        if not 0 <= self.fields[3] <= left < right < self.fields[6] <= 65535:
            raise ValueError(get('0_con_l_r_nfrm_65535'))


class Project:
    def __init__(self, manifest):
        self.path = Path(manifest).resolve()
        self.root = self.path.parent
        self.metadata = json.loads(self.path.read_text(encoding='utf8'))
        if self.metadata.get('version') != 1:
            raise ValueError(get('unsupported_project_version'))
        self.db = self.asset(self.metadata['database'])
        data = self.db.read_bytes()
        if not data or len(data) % RECORD.size:
            raise ValueError(get('invalid_oto_db_size'))
        metadata = {m['symbol']: m for m in self.metadata['entries']}
        self.entries = []
        for row in RECORD.iter_unpack(data):
            entry = Entry(list(row), metadata.get(row[0], {}), tuple(row[4:6]))
            entry.validate(entry.bounds)
            self.entries.append(entry)
        self.disk_hash = digest(data)
        self.undo_stack, self.redo_stack = [], []

    def asset(self, relative):
        result = (self.root / relative).resolve()
        if not result.is_relative_to(self.root):
            raise ValueError(get('project_asset_escapes_project_directory'))
        return result

    @property
    def dirty(self):
        return any(e.dirty for e in self.entries)

    def change(self, edits):
        changes = []
        for index, bounds in edits.items():
            bounds = tuple(map(int, bounds))
            entry = self.entries[index]
            entry.validate(bounds)
            if bounds != entry.bounds:
                changes.append((index, entry.bounds, bounds))
        if changes:
            self._apply(changes, 2)
            self.undo_stack.append(changes)
            self.redo_stack.clear()
        return bool(changes)

    def _apply(self, changes, column):
        for index, before, after in changes:
            self.entries[index].fields[4:6] = (before, after)[column - 1]

    def undo(self):
        if self.undo_stack:
            changes = self.undo_stack.pop()
            self._apply(changes, 1)
            self.redo_stack.append(changes)

    def redo(self):
        if self.redo_stack:
            changes = self.redo_stack.pop()
            self._apply(changes, 2)
            self.undo_stack.append(changes)

    def save(self):
        previous = self.db.read_bytes()
        if digest(previous) != self.disk_hash:
            raise RuntimeError(get('oto_db_changed_externally_reopen_before_saving'))
        for entry in self.entries:
            entry.validate(entry.bounds)
        data = b''.join(RECORD.pack(*e.fields) for e in self.entries)
        atomic_write(self.db.with_suffix('.db.bak'), previous)
        atomic_write(self.db, data)
        self.disk_hash = digest(data)
        for entry in self.entries:
            entry.saved = entry.bounds
