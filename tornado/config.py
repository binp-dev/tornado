# type: ignore
from __future__ import annotations

from ferrite.codegen.types import Int, Float

DAC_COUNT: 'Int(Int.SIZE)' = 1
ADC_COUNT: 'Int(Int.SIZE)' = 6

SAMPLE_FREQ_HZ: 'Int(Int.SIZE)' = 10000

Point = Int(32, signed=True)

DAC_MAX_ABS_V: 'Float(32)' = 10.0
ADC_MAX_ABS_V: 'Float(32)' = 10.0

DAC_STEP_UV: 'Float(32)' = 315.7445
ADC_STEP_UV: 'Float(32)' = 346.8012

DAC_CODE_SHIFT: Point = 32767

MAX_APP_MSG_LEN: 'Int(Int.SIZE)' = 496
MAX_MCU_MSG_LEN: 'Int(Int.SIZE)' = 496

KEEP_ALIVE_PERIOD_MS: 'Int(Int.SIZE)' = 100
KEEP_ALIVE_MAX_DELAY_MS: 'Int(Int.SIZE)' = 200
