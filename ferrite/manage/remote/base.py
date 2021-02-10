
class Device(object):
    def __init__(self):
        super().__init__()

    def name(self) -> str:
        raise NotImplementedError
