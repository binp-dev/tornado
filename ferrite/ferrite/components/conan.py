from __future__ import annotations

from pathlib import Path
from dataclasses import dataclass

from ferrite.utils.run import capture, run
from ferrite.components.base import Context
from ferrite.components.cmake import Cmake
from ferrite.components.toolchain import Toolchain, HostToolchain, CrossToolchain


class ConanProfile:

    def __init__(self, toolchain: Toolchain):
        self.toolchain = toolchain

    def generate(self) -> str:
        tc = self.toolchain

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

        if isinstance(tc, HostToolchain):
            tc_prefix = ""
        elif isinstance(tc, CrossToolchain):
            tc_prefix = f"{tc.path}/bin/{tc.target}-"
        else:
            raise RuntimeError(f"Unsupported toolchain type '{type(tc).__name__}'")

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

        if isinstance(tc, CrossToolchain):
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
    # FIXME: Remove this option
    disable_conan: bool = False

    def configure(self, ctx: Context) -> None:
        if not self.disable_conan:
            self.create_build_dir()

            profile_path = self.build_dir / "profile.conan"
            ConanProfile(self.toolchain).save(profile_path)

            run(
                ["conan", "install", "--build", "missing", self.src_dir, "--profile", profile_path],
                cwd=self.build_dir,
                quiet=ctx.capture,
            )

        super().configure(ctx)
