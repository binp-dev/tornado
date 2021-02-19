from __future__ import annotations
import os
from manage.components.base import Task
from manage.tree import components
from manage.paths import BASE_DIR
from utils.strings import quote

def text_lines(*lines):
    return "".join([l + "\n" for l in lines])

def base_path(path):
    return os.path.relpath(path, BASE_DIR)

class Job(object):
    def __init__(self, task: Task, level: int, deps: list[Job], cached: bool = False):
        super().__init__()
        self.task = task
        self.level = level
        self.deps = deps
        self.cached = cached

    def name(self) -> str:
        return self.task.name()

    def stage(self) -> str:
        return quote(str(self.level))

    def text(self) -> str:
        text = text_lines(
            f"{self.name()}:",
            f"  stage: {self.stage()}",
            f"  script:",
            f"    - python3 -u -m manage {self.name()}",
        )

        if len(self.deps) > 0:
            text += "  needs:\n"
            for dep in self.deps:
                text += f"    - {dep.name()}\n"

        if not self.cached and len(self.task.artifacts()) > 0:
            text += text_lines(
                "  artifacts:",
                "    paths:",
            )
            for art in self.task.artifacts():
                text += f"      - {base_path(art)}\n"

        cache = set()
        for dep in self.deps:
            if dep.cached:
                cache = cache.union(set(dep.task.artifacts()))
        if self.cached:
            cache = cache.union(set(self.task.artifacts()))

        if len(cache) > 0:
            text += text_lines(
                "  cache:",
                "    paths:",
            )
            for cch in cache:
                text += f"      - {base_path(cch)}\n"

        return text

class Graph(object):
    def __init__(self, is_cached):
        self.jobs = {}
        self.is_cached = is_cached

    def add(self, task: Task, level: int = 0) -> Job:
        name = task.name()
        if name in self.jobs:
            job = self.jobs[name]
            if job.level >= level:
                return job
        deps = []
        for dep in task.dependencies():
            deps.append(self.add(dep, level + 1))
        job = Job(task, level, deps, cached=self.is_cached(task.name()))
        self.jobs[name] = job
        return job

    def text(self) -> str:
        text = ""

        stages = sorted(set([str(x.level) for x in self.jobs.values()]))
        text += "\nstages:\n"
        for stg in reversed(stages):
            text += f"  - {quote(stg)}\n"

        sequence = [j for j in sorted(self.jobs.values(), key=lambda j: -j.level)]
        for job in sequence:
            text += "\n"
            text += job.text()
        
        return text


if __name__ == "__main__":
    end_tasks = [
        "all.build",
        "all.test_host",
    ]
    cache_patterns = [
        "epics_base.",
        "_toolchain."
    ]

    graph = Graph(
        is_cached = lambda name: any([p in name for p in cache_patterns]),
    )
    for etn in end_tasks:
        cn, tn = etn.split(".")
        task = components[cn].tasks()[tn]
        graph.add(task)

    text = "image: agerasev/debian-psc\n"

    text += graph.text()

    print(text)
