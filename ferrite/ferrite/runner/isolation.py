from __future__ import annotations
from typing import Generator, List, Optional

from pathlib import Path
from contextlib import contextmanager

from ferrite.components.base import Artifact, Task

from .utils import filter_parents, rename_mkdir, remove_empty_tree


@contextmanager
def isolated(target_dir: Path, hideout_dir: Path) -> Generator[None, None, None]:
    if hideout_dir.exists():
        assert len(list(hideout_dir.iterdir())) == 0
    else:
        hideout_dir.mkdir()
    for path in target_dir.iterdir():
        if path == hideout_dir:
            continue
        relpath = path.relative_to(target_dir)
        path.rename(hideout_dir / relpath)
    exc: Optional[Exception] = None
    try:
        yield
    except Exception as e:
        exc = e
    finally:
        for path in hideout_dir.iterdir():
            relpath = path.relative_to(hideout_dir)
            hidden_path = target_dir / relpath
            if hidden_path.exists():
                if exc is None:
                    exc = FileExistsError(f"Artifact '{hidden_path}' already exists")
                continue
            path.rename(hidden_path)
        if len(list(hideout_dir.iterdir())) == 0:
            hideout_dir.rmdir()
        if exc is not None:
            raise exc


@contextmanager
def with_artifacts(target_dir: Path, hideout_dir: Path, task: Task) -> Generator[None, None, None]:
    artifacts: List[Artifact] = filter_parents([
        *[art for art in task.artifacts()],
        *[art for dep in task.dependencies() for art in dep.artifacts()],
    ], lambda art: art.path)
    print(f"Artifacts: {[str(a.path) for a in artifacts]}")

    for art in artifacts:
        hidden_path = hideout_dir / art.path
        if not hidden_path.exists():
            #raise FileNotFoundError(f"There's no artifact '{art.path}'")
            continue
        rename_mkdir(hidden_path, target_dir / art.path)
    exc: Optional[Exception] = None
    try:
        yield
    except Exception as e:
        exc = e
    finally:
        for art in artifacts:
            real_art_path = target_dir / art.path
            if not real_art_path.exists():
                if exc is None:
                    exc = FileNotFoundError(f"There's no artifact '{art.path}'")
                continue
            hidden_path = hideout_dir / art.path
            if hidden_path.exists():
                if exc is None:
                    exc = FileExistsError(f"Artifact '{art.path}' already exists")
                continue
            rename_mkdir(real_art_path, hidden_path)

        # Cleanup
        for path in target_dir.iterdir():
            if path == hideout_dir:
                continue
            remove_empty_tree(path)

        if exc is not None:
            raise exc
