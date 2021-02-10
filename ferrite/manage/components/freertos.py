from __future__ import annotations
from manage.components.git import Repo

class Freertos(Repo):
    def __init__(self):
        super().__init__(
            "https://github.com/varigit/freertos-variscite.git",
            "freertos",
            "freertos_bsp_1.0.1_imx7d-var01",
        )
