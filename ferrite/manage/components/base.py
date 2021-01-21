from __future__ import annotations

class TaskArgs(object):
    def __init__(self):
        super().__init__()

class Task(object):
    def __init__(self):
        super().__init__()

    # TODO: Return status
    def run(self, args: TaskArgs):
        raise NotImplementedError

class Component(object):
    def __init__(self):
        super().__init__()

    def tasks(self) -> dict[str, Task]:
        raise NotImplementedError
