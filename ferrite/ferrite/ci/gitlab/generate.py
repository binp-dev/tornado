from __future__ import annotations
from typing import Dict, List, Optional, Set, Tuple, Union

import os
import zlib
from pathlib import Path
from dataclasses import dataclass

from ferrite.components.base import Artifact, Component, Task
from ferrite.utils.strings import quote


@dataclass
class Context:
    module: str


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

    def text(self, base_dir: Path, cache: Optional[Cache]) -> List[str]:
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
                *[f"    - {n}" for n in sorted([dj.name() for dj in self.needs()])],
            ])

        if len(self.artifacts()) > 0:
            paths = sorted([str(art.path.relative_to(base_dir)) for art in self.artifacts()])
            lines.extend([
                "  artifacts:",
                "    paths:",
                *[f"      - {art}" for art in paths],
            ])
            del paths

        cached_artifacts = [art.path for art in self.artifacts() if art.cached]
        if len(cached_artifacts) > 0:
            lines.append("  cache:")
            if cache is not None:
                lines.append(f"    - *{cache.name}")
            paths = sorted([str(path.relative_to(base_dir)) for path in cached_artifacts])
            hash = zlib.adler32("\n".join(paths).encode("utf-8"))
            lines.extend([
                f"    - key: \"{self.name()}:{hash:x}\"",
                "      paths:",
                *[f"        - {p}" for p in paths],
            ])
            del paths

        for k, v in self.attributes().items():
            if isinstance(v, str):
                sv = quote(v)
            else:
                sv = str(v).lower()
            lines.append(f"  {k}: {sv}")

        return lines


@dataclass
class TaskJob(Job):
    context: Context
    task: Task
    level: int
    deps: List[Job]

    def name(self) -> str:
        return self.task.name()

    def stage(self) -> int:
        return self.level

    def script(self) -> List[str]:
        return [f"poetry run python -u -m {self.context.module}.manage --no-deps --no-capture {self.name()}"]

    def needs(self) -> List[Job]:
        return self.deps

    def artifacts(self) -> List[Artifact]:
        return self.task.artifacts()


@dataclass
class ScriptJob(Job):
    _name: str
    _stage: int
    _script: List[str]
    allow_failure: bool = False

    def name(self) -> str:
        return self._name

    def stage(self) -> int:
        return self._stage

    def script(self) -> List[str]:
        return self._script

    def attributes(self) -> Dict[str, Job.Attribute]:
        attrs: Dict[str, Job.Attribute] = {}
        if self.allow_failure:
            attrs["allow_failure"] = True
        return attrs


class Graph:

    def __init__(self, context: Context) -> None:
        self.context = context
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
        job = TaskJob(self.context, task, level, deps)
        self.add_job(job)
        return job

    def add_job(self, job: Job) -> None:
        assert job.name() not in self.jobs
        self.jobs[job.name()] = job

    def stages(self) -> Set[int]:
        return set([x.stage() for x in self.jobs.values()])

    def text(self, base_dir: Path, cache: Optional[Cache]) -> List[str]:
        lines: List[str] = []

        lines.extend([
            "stages:",
            *[f"  - \"{stg}\"" for stg in sorted(list(self.stages()))],
        ])

        sequence = [j for j in sorted(self.jobs.values(), key=lambda j: j.stage())]
        for job in sequence:
            lines.append("")
            lines.extend(job.text(base_dir, cache))

        return lines


def make_graph(context: Context, components: Component, end_tasks: List[str]) -> Graph:
    graph = Graph(context)

    for task_name in end_tasks:
        task = components.tasks()[task_name]
        graph.add_task_with_deps(task)

    stage = min(graph.stages()) - 1
    graph.add_job(ScriptJob("yapf", stage, [f"poetry run yapf --diff --recursive {context.module}"], allow_failure=True))
    graph.add_job(ScriptJob("mypy", stage, [f"poetry run mypy -p {context.module}"], allow_failure=True))

    return graph


def generate(
    base_dir: Path,
    graph: Graph,
    vars: Optional[Variables] = None,
    cache: Optional[Cache] = None,
) -> str:
    return "\n".join([
        f"# This file is generated by script '{Path(__file__).relative_to(base_dir)}'",
        "",
        "image: agerasev/debian-psc",
        "",
        *(vars.text() if vars is not None else []),
        "",
        *(cache.text() if cache is not None else []),
        "",
        *graph.text(base_dir, cache),
        "",
    ])


def default_variables() -> Variables:
    return Variables({
        "GIT_SUBMODULE_STRATEGY": "recursive",
        "POETRY_VIRTUALENVS_IN_PROJECT": "true",
    })


def default_cache(lock_deps: bool = False) -> Cache:
    if not lock_deps:
        poetry = (["pyproject.toml"], ["poetry.lock", ".venv/"])
    else:
        poetry = (["pyproject.toml", "poetry.lock"], [".venv/"])

    return Cache("global_cache", [
        poetry,
    ])


def main(
    module: str,
    base_dir: Path,
    components: Component,
    end_tasks: List[str],
    variables: Variables,
    cache: Cache,
) -> None:
    context = Context(module)

    print("Generating script ...")
    text = generate(
        base_dir,
        make_graph(context, components, end_tasks),
        vars=variables,
        cache=cache,
    )

    path = ".gitlab-ci.yml"
    print(f"Writing to '{path}' ...")
    with open(path, "w") as f:
        f.write(text)

    print("Done.")


if __name__ == "__main__":
    from ferrite.manage.tree import make_components

    end_tasks = [
        "all.test",
    ]

    base_dir = Path.cwd()
    target_dir = base_dir / "target"
    components = make_components(base_dir, target_dir)

    main("ferrite", base_dir, components, end_tasks, default_variables(), default_cache())
