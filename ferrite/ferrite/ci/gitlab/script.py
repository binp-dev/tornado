from __future__ import annotations
from typing import Dict, List, Optional, Sequence, Set, Tuple, Union

import os
import zlib
from dataclasses import dataclass

from ferrite.components.base import Artifact, Task
from ferrite.manage.tree import COMPONENTS
from ferrite.manage.paths import BASE_DIR
from ferrite.utils.strings import quote


def base_path(path: str) -> str:
    return os.path.relpath(path, BASE_DIR)


@dataclass
class Variables:
    vars: Dict[str, str]

    def text(self) -> List[str]:
        lines = []
        if len(self.vars) > 0:
            lines.extend([
                "variables:",
                *[f"  {k}: \"{v}\"" for k, v in self.vars.items()],
            ])
        return lines


@dataclass
class Cache:
    name: str
    patterns: List[Tuple[List[str], List[str]]]

    def text(self) -> List[str]:
        lines = []
        if len(self.patterns) > 0:
            lines.append(f"cache: &{self.name}")
            for keys, paths in self.patterns:
                lines.extend([
                    "  - key:",
                    "      files:",
                    *[f"        - {k}" for k in keys],
                    "    paths:",
                    *[f"      - {p}" for p in paths],
                ])
        return lines


class Job:
    Attribute = Union[bool, int, float, str]

    def name(self) -> str:
        raise NotImplementedError()

    def stage(self) -> int:
        raise NotImplementedError()

    def script(self) -> List[str]:
        raise NotImplementedError()

    def needs(self) -> List[Job]:
        return []

    def artifacts(self) -> List[Artifact]:
        return []

    def attributes(self) -> Dict[str, Job.Attribute]:
        return {}

    def text(self, cache: Optional[Cache]) -> List[str]:
        lines = [
            f"{self.name()}:",
            f"  stage: \"{self.stage()}\"",
            f"  script:",
            f"    - poetry install",
            *[f"    - {sl}" for sl in self.script()],
        ]

        if len(self.needs()) > 0:
            lines.extend([
                "  needs:",
                *[f"    - {dj.name()}" for dj in self.needs()],
            ])

        if len(self.artifacts()) > 0:
            lines.extend([
                "  artifacts:",
                "    paths:",
                *[f"      - {base_path(art.path)}" for art in self.artifacts()],
            ])

        cached_artifacts = [art.path for art in self.artifacts() if art.cached]
        if len(cached_artifacts) > 0:
            lines.append("  cache:")
            if cache is not None:
                lines.append(f"    - *{cache.name}")
            paths = [base_path(path) for path in cached_artifacts]
            hash = zlib.adler32("\n".join(sorted(paths)).encode("utf-8"))
            lines.extend([
                f"    - key: \"{self.name()}:{hash:x}\"",
                "      paths:",
                *[f"        - {p}" for p in paths],
            ])

        for k, v in self.attributes().items():
            if isinstance(v, str):
                sv = quote(v)
            else:
                sv = str(v).lower()
            lines.append(f"  {k}: {sv}")

        return lines


class TaskJob(Job):

    def __init__(self, task: Task, level: int, deps: List[Job]):
        super().__init__()
        self.task = task
        self.level = level
        self.deps = deps
        self.cache = cache

    def name(self) -> str:
        return self.task.name()

    def stage(self) -> int:
        return self.level

    def script(self) -> List[str]:
        return [f"poetry run python -u -m ferrite.manage --no-deps --no-capture {self.name()}"]

    def needs(self) -> List[Job]:
        return self.deps

    def artifacts(self) -> List[Artifact]:
        return self.task.artifacts()


@dataclass
class ScriptJob(Job):
    name_: str
    stage_: int
    script_: List[str]
    allow_failure: bool = False

    def name(self) -> str:
        return self.name_

    def stage(self) -> int:
        return self.stage_

    def script(self) -> List[str]:
        return self.script_

    def attributes(self) -> Dict[str, Job.Attribute]:
        attrs: Dict[str, Job.Attribute] = {}
        if self.allow_failure:
            attrs["allow_failure"] = True
        return attrs


class Graph:

    def __init__(self) -> None:
        self.jobs: Dict[str, Job] = {}

    def add_task_with_deps(self, task: Task) -> TaskJob:
        name = task.name()
        if name in self.jobs:
            job = self.jobs[name]
            assert isinstance(job, TaskJob)
            return job
        deps: List[Job] = []
        level = 0
        for dep in task.dependencies():
            dj = self.add_task_with_deps(dep)
            level = max(level, dj.stage() + 1)
            deps.append(dj)
        job = TaskJob(task, level, deps)
        self.add_job(job)
        return job

    def add_job(self, job: Job) -> None:
        assert job.name() not in self.jobs
        self.jobs[job.name()] = job

    def stages(self) -> Set[int]:
        return set([x.stage() for x in self.jobs.values()])

    def text(self, cache: Optional[Cache]) -> List[str]:
        lines: List[str] = []

        lines.extend([
            "stages:",
            *[f"  - \"{stg}\"" for stg in sorted(list(self.stages()))],
        ])

        sequence = [j for j in sorted(self.jobs.values(), key=lambda j: j.stage())]
        for job in sequence:
            lines.append("")
            lines.extend(job.text(cache))

        return lines


def make_graph(end_tasks: List[str]) -> Graph:
    graph = Graph()

    for etn in end_tasks:
        cn, tn = etn.split(".")
        task = COMPONENTS[cn].tasks()[tn]
        graph.add_task_with_deps(task)

    stage = min(graph.stages()) - 1
    graph.add_job(ScriptJob("yapf", stage, ["poetry run yapf --diff --recursive ferrite"], allow_failure=True))
    graph.add_job(ScriptJob("mypy", stage, ["poetry run mypy -p ferrite"], allow_failure=True))

    return graph


def generate(
    graph: Graph,
    vars: Optional[Variables] = None,
    cache: Optional[Cache] = None,
) -> str:
    return "\n".join([
        f"# This file is generated by script '{base_path(__file__)}'",
        "",
        "image: agerasev/debian-psc",
        "",
        *(vars.text() if vars is not None else []),
        "",
        *(cache.text() if cache is not None else []),
        "",
        *graph.text(cache),
        "",
    ])


if __name__ == "__main__":
    end_tasks = [
        "host_all.test",
        #"imx7_all.build",
        "imx8mn_all.build",
    ]
    vars = Variables({
        "POETRY_VIRTUALENVS_IN_PROJECT": "true",
    })
    cache = Cache("global_cache", [
        (["pyproject.toml"], ["poetry.lock", ".venv/"]),
    ])

    print("Generating script ...")
    text = generate(make_graph(end_tasks), vars=vars, cache=cache)

    path = ".gitlab-ci.yml"
    print(f"Writing to '{path}' ...")
    with open(path, "w") as f:
        f.write(text)

    print("Done.")
