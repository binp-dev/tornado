from ferrite.components.base import Task, Context
from .base import Device


class RebootTask(Task):

    def __init__(self):
        super().__init__()

    def run(self, ctx: Context) -> bool:
        assert ctx.device is not None
        ctx.device.reboot()
