from __future__ import annotations
from ferrite.components.git import RepoList, RepoSource


class Freertos(RepoList):

    def __init__(self, name, branch):
        super().__init__(
            name,
            [
                RepoSource("https://gitlab.inp.nsk.su/psc/freertos-variscite.git", branch),
                RepoSource("https://github.com/varigit/freertos-variscite.git", branch),
            ],
        )
        self.name = name
        self.branch = branch


class FreertosImx7(Freertos):

    def __init__(self):
        name = "freertos_bsp_1.0.1_imx7d-var01"
        branch = name
        super().__init__(name, branch)


class FreertosImx8mn(Freertos):

    def __init__(self):
        name = "mcuxpresso_sdk_2.9.x-var01"
        branch = name
        super().__init__(name, branch)
