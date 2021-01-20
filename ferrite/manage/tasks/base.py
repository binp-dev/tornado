from __future__ import annotations

class Task(object):
    def __init__(self):
        super().__init__()

    def dependencies(self) -> list[Task]:
        return []

    def run(self, args: dict[str, str]):
        raise NotImplementedError
