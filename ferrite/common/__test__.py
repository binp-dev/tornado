class A:
    def __init__(self):
        pass

class B(A):
    def __init__(self):
        A.__init__(self)

class C(A):
    def __init__(self, b: bool):
        if b:
            B.__init__(self)
        else:
            A.__init__(self)

cba = C(True)
assert isinstance(cba, C)
assert isinstance(cba, B)
assert isinstance(cba, A)

ca = C(False)
assert isinstance(ca, C)
assert not isinstance(ca, B)
assert isinstance(ca, A)
