from __future__ import annotations
import os
import shutil
import logging
from utils.run import run, RunError
from manage.components.base import Component, Task, Context
from manage.paths import TARGET_DIR

def clone(path: str, remote: str, branch: str = None, clean: bool = False) -> str:
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

class GitCloneTask(Task):
    def __init__(self, path: str, sources: list[(str, str)]):
        super().__init__()
        self.path = path
        self.sources = sources

    def run(self, ctx: Context) -> bool:
        last_error = None
        for remote, branch in self.sources:
            try:
                return clone(self.path, remote, branch, clean=True)
            except RunError as e:
                last_error = e
                continue
        if last_error is not None:
            raise last_error

    def artifacts(self) -> str[list]:
        return [self.path]

class RepoList(Component):
    def __init__(self, name: str, sources: list[(str, str)]):
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
        super().__init__(name, [remote, branch])
