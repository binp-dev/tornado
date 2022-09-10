from __future__ import annotations
from typing import Dict, List, Optional

import shutil
from pathlib import Path

from dataclasses import dataclass
from ferrite.utils.run import run, RunError
from ferrite.components.base import Artifact, Component, Task, Context

import logging

logger = logging.getLogger(__name__)


def clone(path: Path, remote: str, branch: Optional[str] = None, clean: bool = False, quiet: bool = False) -> bool:
    if path.exists():
        # FIXME: Pull if update available
        logger.info(f"Repo '{remote}' is cloned already")
        return False
    try:
        path.parent.mkdir(exist_ok=True, parents=True)
        run(
            ["git", "clone", remote, path.name],
            cwd=path.parent,
            add_env={"GIT_TERMINAL_PROMPT": "0"},
            quiet=quiet,
        )
        if branch:
            run(["git", "checkout", branch], cwd=path, quiet=quiet)
        run(["git", "submodule", "update", "--init", "--recursive"], cwd=path, quiet=quiet)
    except RunError:
        if path.exists():
            shutil.rmtree(path)
        raise
    if clean:
        shutil.rmtree(path / ".git")
    return True


@dataclass
class RepoSource:
    remote: str
    branch: Optional[str]

    def __repr__(self) -> str:
        return f"'{self.remote}'" + f", branch '{self.branch}'" if self.branch is not None else ""


@dataclass(eq=False)
class GitCloneTask(Task):

    path: Path
    sources: List[RepoSource]
    cached: bool = False

    def run(self, ctx: Context) -> None:
        last_error = None
        for source in self.sources:
            try:
                clone(self.path, source.remote, source.branch, clean=True, quiet=ctx.capture)
                return
            except RunError as e:
                last_error = e
                logger.warning(f"Failed to clone {source}: {e}")
                continue
        if last_error is not None:
            raise last_error

    def artifacts(self) -> List[Artifact]:
        return [Artifact(self.path, cached=self.cached)]


class RepoList(Component):

    def __init__(self, dst_dir: Path, sources: List[RepoSource], cached: bool = False):
        super().__init__()

        self.path = dst_dir
        self.sources = sources
        self.cached = cached

        self.clone_task = GitCloneTask(self.path, self.sources, cached=cached)


class Repo(RepoList):

    def __init__(self, dst_dir: Path, remote: str, branch: Optional[str] = None):
        super().__init__(dst_dir, [RepoSource(remote, branch)])
