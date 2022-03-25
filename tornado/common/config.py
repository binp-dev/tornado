from __future__ import annotations

from pathlib import Path
import pydantic

from ferrite.utils.interop import read_defs


class Config(pydantic.BaseModel):
    adc_count: int

    sample_freq_hz: float

    dac_max_abs_v: float
    adc_max_abs_v: float

    dac_code_shift: int
    dac_step_uv: float
    adc_step_uv: float

    rpmsg_max_app_msg_len: int
    rpmsg_max_mcu_msg_len: int

    keep_alive_period_ms: int
    keep_alive_max_delay_ms: int


def read_common_config(source_dir: Path) -> Config:
    defs = read_defs(source_dir / "common" / "include" / "common" / "config.h")
    obj = {k.lower(): v for k, v in defs.items()}
    return Config.parse_obj(obj)
