from __future__ import annotations
import shutil
from typing import Dict, List, cast

from pathlib import Path
from dataclasses import dataclass
import toml
import pydantic

from ferrite.utils.run import capture, run
from ferrite.components.base import Context
from ferrite.components.cmake import Cmake, CmakeRunnable
from ferrite.components.compiler import Gcc, GccHost, GccCross


@dataclass
class Conanfile:
    requires: List[str]
    generators: List[str]

    def dumps(self) -> str:
        return "\n".join([
            "[requires]",
            *self.requires,
            "",
            "[generators]",
            *self.generators,
        ])

    def save(self, path: Path) -> None:
        with open(path, "w") as f:
            f.write(self.dumps())


@dataclass
class _ConanfileExtCollapsed:
    requires: Dict[str, str]

    def update(self, other: _ConanfileExtCollapsed) -> None:
        # TODO: Compare versions on collision
        assert len(set(self.requires.keys()).intersection(set(other.requires.keys()))) == 0
        self.requires.update(other.requires)

    def to_conanfile(self) -> Conanfile:
        return Conanfile(
            requires=[f"{k}/{v}" for k, v in self.requires.items()],
            generators=["cmake"],
        )


class _ConanfileExt(pydantic.BaseModel):

    class Dependency(pydantic.BaseModel):
        path: str

    requires: Dict[str, str] = {}
    dependencies: List[_ConanfileExt.Dependency] = []

    def to_collapsed_without_deps(self) -> _ConanfileExtCollapsed:
        return _ConanfileExtCollapsed(requires=dict(self.requires))


_ConanfileExt.update_forward_refs()


def _read_conanfile_ext(base_dir: Path) -> _ConanfileExt:
    path = base_dir / "conanfile.toml"
    with open(path, "r") as f:
        data = toml.load(f)
    return _ConanfileExt.parse_obj(data)


def _read_and_collapse_conanfile_ext(base_dir: Path, vars: Dict[str, str]) -> _ConanfileExtCollapsed:
    raw = _read_conanfile_ext(base_dir)
    collapsed = raw.to_collapsed_without_deps()
    for dep in raw.dependencies:
        tmp = dep.path
        for k, v in vars.items():
            tmp = tmp.replace(f"${k}", v)
        dep_path = Path(tmp)
        if not dep_path.is_absolute():
            dep_path = (base_dir / dep_path).resolve()
        collapsed.update(_read_and_collapse_conanfile_ext(dep_path, vars))
    return collapsed


def make_conanfile(base_dir: Path, vars: Dict[str, str] = {}) -> Conanfile:
    return _read_and_collapse_conanfile_ext(base_dir, vars).to_conanfile()


class ConanProfile:

    def __init__(self, cc: Gcc):
        self.cc = cc

    def generate(self) -> str:
        tc = self.cc

        if tc.target.isa == "x86_64":
            arch = "x86_64"
        elif tc.target.isa == "arm":
            arch = "armv7"
        elif tc.target.isa == "aarch64":
            arch = "armv8"
        else:
            raise RuntimeError(f"Unsupported arch '{tc.target.isa}'")

        if tc.target.api == "linux":
            os = "Linux"
        else:
            raise RuntimeError(f"Unsupported os '{tc.target.api}'")

        if isinstance(tc, GccHost):
            tc_prefix = ""
        elif isinstance(tc, GccCross):
            tc_prefix = f"{tc.path}/bin/{tc.target}-"
        else:
            raise RuntimeError(f"Unsupported cc type '{type(tc).__name__}'")

        ver = capture([f"{tc_prefix}gcc", "-dumpversion"]).split(".")
        version = ".".join(ver[:min(len(ver), 2)])

        content = [
            f"[settings]",
            f"os={os}",
            f"arch={arch}",
            f"compiler=gcc",
            f"compiler.version={version}",
            f"compiler.libcxx=libstdc++11",
            f"build_type=Release",
        ]

        if isinstance(tc, GccCross):
            content += [
                f"[env]",
                f"CONAN_CMAKE_FIND_ROOT_PATH={tc.path}",
                f"CHOST={tc.target}",
                f"CC={tc_prefix}gcc",
                f"CXX={tc_prefix}g++",
                f"AR={tc_prefix}ar",
                f"AS={tc_prefix}as",
                f"RANLIB={tc_prefix}ranlib",
                f"STRIP={tc_prefix}strip",
            ]

        return "\n".join(content)

    def save(self, path: Path) -> None:
        with open(path, "w") as f:
            f.write(self.generate())


@dataclass
class CmakeWithConan(Cmake):

    def configure(self, capture: bool = False) -> None:
        self.create_build_dir()

        profile_path = self.build_dir / "profile.conan"
        ConanProfile(self.cc).save(profile_path)

        conanfile_path = self.build_dir / "conanfile.txt"
        try:
            # Try to find and copy already ready conanfile
            shutil.copyfile(self.src_dir / "conanfile.txt", conanfile_path)
        except FileNotFoundError:
            # Generate conanfile
            make_conanfile(self.src_dir, self._defs).save(conanfile_path)

        run(
            ["conan", "install", conanfile_path, "--profile", profile_path, "--build", "missing"],
            cwd=self.build_dir,
            quiet=capture,
        )

        super().configure(capture=capture)


@dataclass
class CmakeRunnableWithConan(CmakeRunnable, CmakeWithConan):
    pass
