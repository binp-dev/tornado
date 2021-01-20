from __future__ import annotations

class TaskArgs(object):
    def __init__(self):
        super().__init__()

class Task(object):
    def __init__(self):
        super().__init__()

    def run(self, args: TaskArgs):
        raise NotImplementedError

class EmptyArgs(TaskArgs):
    def __init__(self):
        super().__init__()
