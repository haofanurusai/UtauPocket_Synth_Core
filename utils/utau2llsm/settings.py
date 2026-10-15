"""Backward-compatible JSON preferences, separate from database dirty state."""
from pathlib import Path
import json
from project import write_json


class Settings(dict):
    def __init__(self, path='config.json'):
        super().__init__()
        self.path = Path(path)
        try:
            self.update(json.loads(self.path.read_text(encoding='utf8')))
        except (OSError, ValueError):
            pass

    def save(self):
        write_json(self.path, self)
