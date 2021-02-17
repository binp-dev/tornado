from __future__ import annotations
from manage.components.git import RepoList

class Freertos(RepoList):
    def __init__(self):
        branch = "freertos_bsp_1.0.1_imx7d-var01"
        super().__init__(
            "freertos",
            [
                ("https://gitlab.inp.nsk.su/psc/freertos-variscite.git", branch),
                ("https://github.com/varigit/freertos-variscite.git", branch),
            ],
        )
        self.branch = branch
