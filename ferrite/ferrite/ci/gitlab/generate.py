from __future__ import annotations
from typing import Dict, List, Literal, Set, Union

from pathlib import Path, PurePath
from dataclasses import dataclass
from graphlib import TopologicalSorter

from ferrite.components.base import Task
from ferrite.utils.strings import quote


@dataclass
class Context:
    base_dir: PurePath
    target_dir: PurePath


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
    _name: str
    paths: List[PurePath]

    @property
    def name(self) -> str:
        return f"{self._name}_cache"

    def generate(self) -> List[str]:
        if len(self.paths) > 0:
            return [
                f"cache: &{self.name}",
                f"  - key: \"{self._name}\"",
                f"    paths:",
                *[f"      - {str(p)}" for p in self.paths],
            ]
        else:
            return []


stage_list = ["self_check", "host_test", "cross_build"]
Stage = Union[Literal["self_check"], Literal["host_test"], Literal["cross_build"]]

Attribute = Union[bool, int, float, str]


@dataclass
class Job:
    stage: Stage

    def name(self) -> str:
        raise NotImplementedError()

    def script(self, ctx: Context) -> List[str]:
        raise NotImplementedError()

    def needs(self) -> List[Job]:
        return []

    def cache(self) -> List[PurePath]:
        return []

    def artifacts(self) -> List[PurePath]:
        return []

    def attributes(self) -> Dict[str, Attribute]:
        return {}

    def generate(self, ctx: Context, global_cache: Cache) -> List[str]:
        lines = [
            f"{self.name()}:",
            f"  stage: \"{self.stage}\"",
        ]
        script = [
            *([f"cd {str(ctx.base_dir)}"] if len(ctx.base_dir.parts) != 0 else []),
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
                *[f"    - {n}" for n in sorted([dj.name() for dj in self.needs()])],
            ])

        if len(self.artifacts()) > 0:
            paths = sorted([str(art) for art in self.artifacts()])
            lines.extend([
                "  artifacts:",
                "    paths:",
                *[f"      - {ctx.target_dir / art}" for art in paths],
            ])
            del paths

        lines.append("  cache:")
        lines.append(f"    - *{global_cache.name}")
        cache = self.cache()
        if len(cache) > 0:
            lines.extend([
                "    - key: \"$CI_JOB_NAME\"",
                "      paths:",
            ])
            for path in sorted(cache):
                lines.append(f"        - {str(ctx.target_dir / path)}")

        for k, v in self.attributes().items():
            if isinstance(v, str):
                sv = quote(v)
            else:
                sv = str(v).lower()
            lines.append(f"  {k}: {sv}")

        return lines


@dataclass
class TaskJob(Job):
    module: str
    task: Task
    deps: List[Job]

    def name(self) -> str:
        return f"{self.module}.{self.task.name()}"

    def script(self, ctx: Context) -> List[str]:
        return [f"poetry run python -u -m {self.module}.manage --no-capture --local --hide-artifacts -j4 {self.task.name()}"]

    def needs(self) -> List[Job]:
        return self.deps

    def artifacts(self) -> List[PurePath]:
        return list({art.path.pure() for art in self.task.artifacts()})

    def cache(self) -> List[PurePath]:
        paths: Set[PurePath] = set()
        for task in TopologicalSorter(self.task.graph()).static_order():
            for art in task.artifacts():
                if art.cached:
                    paths.add(art.path.pure())
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

    def attributes(self) -> Dict[str, Attribute]:
        attrs: Dict[str, Attribute] = {}
        if self.allow_failure:
            attrs["allow_failure"] = True
        return attrs


def generate(
    ctx: Context,
    jobs: List[Job],
    cache: Cache,
    vars: Variables,
    image_version: str = "latest",
) -> str:
    print("Generating CI script ...")

    return "\n".join([
        f"image: agerasev/debian-psc:{image_version}",
        "",
        *(vars.generate() if vars is not None else []),
        "",
        *cache.generate(),
        "",
        "stages:",
        *[f"  - \"{st}\"" for st in stage_list],
        "",
        *["\n".join(j.generate(ctx, cache)) + "\n" for j in jobs],
    ])


def default_variables() -> Variables:
    return Variables({
        "GIT_SUBMODULE_STRATEGY": "recursive",
        "POETRY_VIRTUALENVS_IN_PROJECT": "true",
    })


def default_cache(name: str = "global", lock_deps: bool = True) -> Cache:
    if not lock_deps:
        poetry = [PurePath("poetry.lock"), PurePath(".venv/")]
    else:
        poetry = [PurePath(".venv/")]

    return Cache(name, [
        *poetry,
        PurePath(".cargo"),
    ])


def write_to_file(text: str, script: Path, path: Path = Path.cwd() / ".gitlab-ci.yml") -> None:
    print(f"Writing to '{path}' ...")

    with open(path, "w") as f:
        f.write(
            "\n".join([
                f"# This file is generated by script '{script.relative_to(path.parent)}'",
                "# Please, do not edit it manually",
                "\n",
            ])
        )
        f.write(text)


if __name__ == "__main__":
    from ferrite.components.tree import make_components as make_ferrite_components
    from example.components.tree import make_components as make_example_components

    ferrite_tasks = make_ferrite_components().tasks()
    example_tasks = make_example_components().tasks()
    jobs = [
        ScriptJob("self_check", "pytest", [f"poetry run bash scripts/pytest.sh"]),
        ScriptJob("self_check", "mypy", [f"poetry run bash scripts/mypy.sh"], allow_failure=True),
        TaskJob("host_test", "ferrite", ferrite_tasks["all.test"], []),
        TaskJob("host_test", "example", example_tasks["host.all.test"], []),
        TaskJob("cross_build", "example", example_tasks["arm.all.build"], []),
        TaskJob("cross_build", "example", example_tasks["aarch64.all.build"], []),
    ]

    text = generate(
        Context(PurePath(), PurePath("target")),
        jobs,
        cache=default_cache(),
        vars=default_variables(),
        image_version="0.3",
    )

    write_to_file(text, Path(__file__))

    print("Done.")
