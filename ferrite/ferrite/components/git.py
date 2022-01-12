from __future__ import annotations
from typing import Dict, List, Optional

import shutil
import logging
from pathlib import Path

from dataclasses import dataclass
from ferrite.utils.run import run, RunError
from ferrite.components.base import Artifact, Component, Task, Context


def clone(path: Path, remote: str, branch: Optional[str] = None, clean: bool = False, quiet: bool = False) -> bool:
    if path.exists():
        # FIXME: Pull if update available
        logging.info(f"Repo '{remote}' is cloned already")
        return False
    try:
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


class GitCloneTask(Task):

    def __init__(self, path: Path, sources: List[RepoSource], cached: bool = False):
        super().__init__()
        self.path = path
        self.sources = sources
        self.cached = cached

    def run(self, ctx: Context) -> None:
        last_error = None
        for source in self.sources:
            try:
                clone(self.path, source.remote, source.branch, clean=True, quiet=ctx.capture)
                return
            except RunError as e:
                last_error = e
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

    def tasks(self) -> Dict[str, Task]:
        return {
            "clone": self.clone_task,
        }


class Repo(RepoList):

    def __init__(self, dst_dir: Path, remote: str, branch: Optional[str] = None):
        super().__init__(dst_dir, [RepoSource(remote, branch)])
