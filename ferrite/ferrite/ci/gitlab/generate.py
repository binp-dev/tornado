from __future__ import annotations
from typing import Dict, List, Optional, Set, Union

from pathlib import Path
from dataclasses import dataclass
from graphlib import TopologicalSorter

from ferrite.components.base import Component, Task
from ferrite.utils.strings import quote


@dataclass
class Context:
    module: str
    base_dir: Path


@dataclass
class Variables:
    vars: Dict[str, str]

    def generate(self) -> List[str]:
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
    paths: List[Path]

    def generate(self, ctx: Context) -> List[str]:
        if len(self.paths) > 0:
            return [
                f"cache: &{self.name}",
                f"  - paths:",
                *[f"    - {str(p.relative_to(ctx.base_dir))}" for p in self.paths],
            ]
        else:
            return []


class Job:
    Attribute = Union[bool, int, float, str]

    def name(self) -> str:
        raise NotImplementedError()

    def script(self, ctx: Context) -> List[str]:
        raise NotImplementedError()

    def needs(self) -> List[Job]:
        return []

    def cache(self) -> List[Path]:
        return []

    def artifacts(self) -> List[Path]:
        return []

    def attributes(self) -> Dict[str, Job.Attribute]:
        return {}

    def generate(self, ctx: Context, global_cache: List[Cache] = []) -> List[str]:
        lines = [
            f"{ctx.module}.{self.name()}:",
            f"  stage: \"main\"",
        ]
        script = [
            *([f"cd {str(Path.cwd().relative_to(ctx.base_dir))}"] if ctx.base_dir != Path.cwd() else []),
            "poetry install",
            *self.script(ctx),
        ]
        lines.extend([
            "  script:",
            *[f"    - {sl}" for sl in script],
        ])

        if len(self.needs()) > 0:
            lines.extend([
                "  needs:",
                *[f"    - {ctx.module}.{n}" for n in sorted([dj.name() for dj in self.needs()])],
            ])

        if len(self.artifacts()) > 0:
            paths = sorted([str(art.relative_to(ctx.base_dir)) for art in self.artifacts()])
            lines.extend([
                "  artifacts:",
                "    paths:",
                *[f"      - {art}" for art in paths],
            ])
            del paths

        cache = self.cache()
        if len(cache) + len(global_cache) > 0:
            lines.append("  cache:")
            for name in sorted([c.name for c in global_cache]):
                lines.append(f"    - *{name}")

            if len(cache) > 0:
                lines.append("    - paths:")
                for path in sorted(cache):
                    lines.append(f"      - {str(path.relative_to(ctx.base_dir))}")

        for k, v in self.attributes().items():
            if isinstance(v, str):
                sv = quote(v)
            else:
                sv = str(v).lower()
            lines.append(f"  {k}: {sv}")

        return lines


@dataclass
class TaskJob(Job):
    task: Task
    deps: List[Job]

    def name(self) -> str:
        return self.task.name()

    def script(self, ctx: Context) -> List[str]:
        return [f"poetry run python -u -m {ctx.module}.manage --no-capture --hide-artifacts -j1 {self.name()}"]

    def needs(self) -> List[Job]:
        return self.deps

    def artifacts(self) -> List[Path]:
        return list({art.path for art in self.task.artifacts()})

    def cache(self) -> List[Path]:
        paths: Set[Path] = set()
        for task in TopologicalSorter(self.task.graph()).static_order():
            for art in task.artifacts():
                if art.cached:
                    paths.add(art.path)
        return list(paths)


@dataclass
class ScriptJob(Job):
    _name: str
    _script: List[str]
    allow_failure: bool = False

    def name(self) -> str:
        return self._name

    def script(self, ctx: Context) -> List[str]:
        return self._script

    def attributes(self) -> Dict[str, Job.Attribute]:
        attrs: Dict[str, Job.Attribute] = {}
        if self.allow_failure:
            attrs["allow_failure"] = True
        return attrs


def make_jobs(components: Component, tasks: List[str]) -> List[Job]:
    jobs: List[Job] = []

    for name in tasks:
        task = components.tasks()[name]
        jobs.append(TaskJob(task, []))

    return jobs


def generate_local(
    ctx: Context,
    jobs: List[Job],
    cache: List[Cache],
    includes: List[Path],
) -> str:
    return "\n".join([
        *["\n".join(c.generate(ctx)) + "\n" for c in cache],
        *["\n".join(j.generate(ctx, global_cache=cache)) + "\n" for j in jobs],
    ])


def generate(
    ctx: Context,
    jobs: List[Job],
    cache: List[Cache],
    includes: List[Path],
    vars: Optional[Variables] = None,
    image_version: str = "latest",
) -> str:
    return "\n".join([
        f"image: agerasev/debian-psc:{image_version}",
        "",
        *(vars.generate() if vars is not None else []),
        "",
        "stages:",
        "  - \"main\"",
        "",
        "include:",
        *["  - " + str(path.relative_to(ctx.base_dir)) for path in includes],
        "",
        generate_local(ctx, jobs, cache, includes),
    ])


def default_variables() -> Variables:
    return Variables({
        "GIT_SUBMODULE_STRATEGY": "recursive",
        "POETRY_VIRTUALENVS_IN_PROJECT": "true",
    })


def default_cache(name: str = "global_cache", lock_deps: bool = False) -> Cache:
    root = Path.cwd()
    if not lock_deps:
        poetry = [root / "poetry.lock", root / ".venv/"]
    else:
        poetry = [root / ".venv/"]

    return Cache(name, [
        *poetry,
        root / ".cargo",
    ])


if __name__ == "__main__":
    from ferrite.components.tree import make_components

    self_dir = Path.cwd()
    target_dir = self_dir / "target"

    ctx = Context("ferrite", self_dir)

    components = make_components(self_dir, target_dir)
    jobs = [
        TaskJob(components.tasks()["all.test"], []),
        ScriptJob("mypy", [f"poetry run mypy -p {ctx.module}"], allow_failure=True),
    ]

    print("Generating script ...")
    text = generate(
        ctx,
        jobs,
        cache=[default_cache()],
        includes=[Path.cwd() / "example/.gitlab-ci.yml"],
        vars=default_variables(),
        image_version="0.3",
    )

    path = ".gitlab-ci.yml"
    print(f"Writing to '{path}' ...")
    with open(path, "w") as f:
        f.write(f"# This file is generated by script '{Path(__file__).relative_to(self_dir)}'\n")
        f.write(text)

    print("Done.")
