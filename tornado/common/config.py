from __future__ import annotations

from pathlib import Path
import pydantic

from ferrite.utils.interop import read_defs


class Config(pydantic.BaseModel):
    adc_count: int
    rpmsg_max_msg_len: int


def read_common_config(source_dir: Path) -> Config:
    defs = read_defs(source_dir / "common" / "include" / "common" / "config.h")
    obj = {k.lower(): v for k, v in defs.items()}
    return Config.parse_obj(obj)
