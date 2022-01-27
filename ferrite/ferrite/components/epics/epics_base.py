from __future__ import annotations
from typing import Dict, List

import shutil
from pathlib import Path, PurePosixPath
import logging

from ferrite.utils.files import substitute, allow_patterns
from ferrite.components.base import Component, Task, Context
from ferrite.components.git import RepoList, RepoSource
from ferrite.components.toolchain import Toolchain, HostToolchain, CrossToolchain
from ferrite.components.epics.base import EpicsBuildTask, EpicsDeployTask, epics_arch, epics_host_arch, epics_arch_by_target


class EpicsBaseBuildTask(EpicsBuildTask):

    def __init__(
        self,
        src_dir: Path,
        build_dir: Path,
        install_dir: Path,
        deps: List[Task],
        toolchain: Toolchain,
    ):
        super().__init__(
            src_dir,
            build_dir,
            install_dir,
            deps=deps,
            cached=True,
        )
        self.toolchain = toolchain

    def _configure_common(self) -> None:
        defs = [
            #("USR_CFLAGS", ""),
            #("USR_CPPFLAGS", ""),
            ("USR_CXXFLAGS", "-std=c++17"),
            ("BIN_PERMISSIONS", "755"),
            ("LIB_PERMISSIONS", "644"),
            ("SHRLIB_PERMISSIONS", "755"),
            ("INSTALL_PERMISSIONS", "644"),
        ]
        rules = [(f"^(\\s*{k}\\s*=).*$", f"\\1 {v}") for k, v in defs]
        logging.info(rules)
        substitute(
            rules,
            self.build_dir / "configure/CONFIG_COMMON",
        )

    def _configure_toolchain(self) -> None:
        if isinstance(self.toolchain, HostToolchain):
            substitute(
                [
                    ("^(\\s*CROSS_COMPILER_TARGET_ARCHS\\s*=).*$", "\\1"),
                ],
                self.build_dir / "configure/CONFIG_SITE",
            )

        elif isinstance(self.toolchain, CrossToolchain):
            host_arch = epics_host_arch(self.src_dir)
            cross_arch = epics_arch_by_target(self.toolchain.target)
            if cross_arch == "linux-arm" and host_arch.endswith("-x86_64"):
                host_arch = host_arch[:-3] # Trim '_64'

            substitute(
                [
                    ("^(\\s*CROSS_COMPILER_TARGET_ARCHS\\s*=).*$", f"\\1 {cross_arch}"),
                ],
                self.build_dir / "configure/CONFIG_SITE",
            )

            substitute(
                [
                    ("^(\\s*GNU_TARGET\\s*=).*$", f"\\1 {str(self.toolchain.target)}"),
                    ("^(\\s*GNU_DIR\\s*=).*$", f"\\1 {self.toolchain.path}"),
                ],
                self.build_dir / f"configure/os/CONFIG_SITE.{host_arch}.{cross_arch}",
            )

        else:
            raise RuntimeError(f"Unsupported toolchain type: {type(self.toolchain).__name__}")

    def _configure_install(self) -> None:
        substitute([
            ("^\\s*#*(\\s*INSTALL_LOCATION\\s*=).*$", f"\\1 {self.install_dir}"),
        ], self.build_dir / "configure/CONFIG_SITE")

    def _configure(self) -> None:
        self._configure_common()
        self._configure_toolchain()
        #self._configure_install()

    def _install(self) -> None:
        host_arch = epics_host_arch(self.build_dir)
        arch = epics_arch(self.build_dir, self.toolchain)
        paths = [
            "bin",
            "cfg",
            #"configure",
            "db",
            "dbd",
            #"html",
            "include",
            "lib",
            #"templates",
        ]
        for path in paths:
            shutil.copytree(
                self.build_dir / path,
                self.install_dir / path,
                dirs_exist_ok=True,
                symlinks=True,
                ignore=shutil.ignore_patterns("O.*"),
            )
        if arch != host_arch:
            shutil.copytree(
                self.build_dir / "bin" / host_arch,
                self.install_dir / "bin" / arch,
                dirs_exist_ok=True,
                symlinks=True,
                ignore=allow_patterns("*.pl", "*.py"),
            )

    def run(self, ctx: Context) -> None:
        done_path = self.build_dir / "build.done"
        if done_path.exists():
            with open(done_path, "r") as f:
                if Path(f.read()) == self.build_dir:
                    logging.info(f"'{self.build_dir}' is already built")
                    return

        super().run(ctx)
        with open(done_path, "w") as f:
            f.write(str(self.build_dir))


class EpicsBase(Component):

    def __init__(self) -> None:
        super().__init__()
        self.paths: Dict[str, Path] = {}

    def arch(self) -> str:
        raise NotImplementedError()


class EpicsBaseHost(EpicsBase):

    def __init__(self, target_dir: Path, toolchain: HostToolchain):
        super().__init__()

        self.toolchain = toolchain

        self.version = "7.0.4.1"
        self.prefix = f"epics_base_{self.version}"
        self.src_path = target_dir / f"{self.prefix}_src"
        self.repo = RepoList(
            self.src_path,
            [
                RepoSource("https://gitlab.inp.nsk.su/epics/epics-base.git", f"binp-R{self.version}"),
                RepoSource("https://github.com/epics-base/epics-base.git", f"R{self.version}"),
            ],
            cached=True,
        )

        self.names = {
            "build": f"{self.prefix}_build_{self.toolchain.name}",
            "install": f"{self.prefix}_install_{self.toolchain.name}",
        }
        self.paths = {k: target_dir / v for k, v in self.names.items()}

        self.build_task = EpicsBaseBuildTask(
            self.src_path,
            self.paths["build"],
            self.paths["install"],
            [self.repo.clone_task],
            self.toolchain,
        )

    def arch(self) -> str:
        return epics_host_arch(self.src_path)

    def tasks(self) -> Dict[str, Task]:
        return {
            "clone": self.repo.clone_task,
            "build": self.build_task,
        }


class EpicsBaseCross(EpicsBase):

    def __init__(self, target_dir: Path, toolchain: CrossToolchain, host_base: EpicsBaseHost):
        super().__init__()

        self.toolchain = toolchain
        self.host_base = host_base

        self.prefix = self.host_base.prefix

        self.names = {
            "build": f"{self.prefix}_build_{self.toolchain.name}",
            "install": f"{self.prefix}_install_{self.toolchain.name}",
        }
        self.paths = {k: target_dir / v for k, v in self.names.items()}
        self.deploy_path = PurePosixPath("/opt/epics_base")

        self.build_task = EpicsBaseBuildTask(
            self.host_base.paths["build"],
            self.paths["build"],
            self.paths["install"],
            [
                self.toolchain.download_task,
                self.host_base.build_task,
            ],
            self.toolchain,
        )
        self.deploy_task = EpicsDeployTask(
            self.paths["install"],
            self.deploy_path,
            [self.build_task],
        )

    def arch(self) -> str:
        return epics_arch_by_target(self.toolchain.target)

    def tasks(self) -> Dict[str, Task]:
        return {
            "build": self.build_task,
            "deploy": self.deploy_task,
        }
