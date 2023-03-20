from __future__ import annotations
from typing import Dict

from ferrite.components.base import Context
from ferrite.components.rust import Cargo


class AppBase(Cargo):

    def env(self, ctx: Context) -> Dict[str, str]:
        return {
            **super().env(ctx),
            "TARGET_DIR": str(ctx.target_path),
        }
