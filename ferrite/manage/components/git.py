from __future__ import annotations
from typing import List, Optional, Tuple
import os
import shutil
import logging
from dataclasses import dataclass
from utils.run import run, RunError
from manage.components.base import Component, Task, Context
from manage.paths import TARGET_DIR


def clone(path: str, remote: str, branch: Optional[str] = None, clean: bool = False) -> bool:
    # FIXME: Pull if update available
    if os.path.exists(path):
        logging.info(f"Repo '{remote}' is cloned already")
        return False
    try:
        os.makedirs(TARGET_DIR, exist_ok=True)
        run(
            ["git", "clone", remote, os.path.basename(path)],
            cwd=os.path.dirname(path),
            add_env={"GIT_TERMINAL_PROMPT": "0"},
        )
        if branch:
            run(["git", "checkout", branch], cwd=path)
        run(["git", "submodule", "update", "--init", "--recursive"], cwd=path)
    except RunError:
        if os.path.exists(path):
            shutil.rmtree(path)
        raise
    if clean:
        shutil.rmtree(os.path.join(path, ".git"))
    return True


@dataclass
class Source:
    remote: str
    branch: Optional[str]


class GitCloneTask(Task):

    def __init__(self, path: str, sources: List[Source]):
        super().__init__()
        self.path = path
        self.sources = sources

    def run(self, ctx: Context) -> bool:
        last_error = None
        for source in self.sources:
            try:
                return clone(self.path, source.remote, source.branch, clean=True)
            except RunError as e:
                last_error = e
                continue
        if last_error is not None:
            raise last_error
        return False

    def artifacts(self) -> List[str]:
        return [self.path]


class RepoList(Component):

    def __init__(self, name: str, sources: List[Source]):
        super().__init__()

        self.name = name
        self.path = os.path.join(TARGET_DIR, name)
        self.sources = sources

        self.clone_task = GitCloneTask(self.path, self.sources)

    def tasks(self) -> dict[str, Task]:
        return {
            "clone": self.clone_task,
        }


class Repo(RepoList):

    def __init__(self, name: str, remote: str, branch: str = None):
        super().__init__(name, [Source(remote, branch)])
