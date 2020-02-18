
class Component:
    def setup(self, args, tools):
        raise NotImplementedError()

    def build(self):
        raise NotImplementedError()

    def clean(self):
        raise NotImplementedError()

    def deploy(self):
        raise NotImplementedError()
    
    def test(self):
        raise NotImplementedError()
