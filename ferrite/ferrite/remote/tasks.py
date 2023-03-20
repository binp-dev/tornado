from ferrite.components.base import Task, Context


class RebootTask(Task):

    def __init__(self) -> None:
        super().__init__()

    def run(self, ctx: Context) -> None:
        assert ctx.device is not None
        ctx.device.reboot()
