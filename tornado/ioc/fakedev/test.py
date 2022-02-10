from __future__ import annotations
from typing import List

import time
from math import ceil
from pathlib import Path
import logging

from ferrite.utils.epics.ioc import make_ioc
import ferrite.utils.epics.ca as ca

from tornado.ioc.fakedev.base import FakeDev


def assert_eq(a: float, b: float, eps: float = 1e-3) -> None:
    if abs(a - b) > eps:
        raise AssertionError(f"abs({a} - {b}) < {eps}")


class Handler(FakeDev.Handler):

    def __init__(self) -> None:
        self.adc_count = 6
        self.adc_wf_size = 0
        self.adc_wfs: List[List[int]] = [[] for i in range(self.adc_count)]

        self.dac_wf_size = 0
        self.dac_wfs: List[List[int]] = []

    def _fill_dac_wf_buff(self, dac_wf_buff: List[int], dac_wf_data: List[int], dac_wf_data_pos: int) -> int:
        elems_to_fill = self.dac_wf_size - len(dac_wf_buff)
        elems_left = len(dac_wf_data) - dac_wf_data_pos
        if (elems_left < elems_to_fill):
            elems_to_fill = elems_left

        dac_wf_buff += dac_wf_data[dac_wf_data_pos:dac_wf_data_pos + elems_to_fill]
        return elems_to_fill

    def write_dac_wf(self, dac_wf_data: List[int]) -> None:
        if len(self.dac_wfs) == 0:
            self.dac_wfs.append([])

        dac_wf_data_pos = 0
        dac_wf_buff = self.dac_wfs[-1]
        while dac_wf_data_pos < len(dac_wf_data):
            if len(dac_wf_buff) == self.dac_wf_size:
                self.dac_wfs.append([])
                dac_wf_buff = self.dac_wfs[-1]

            dac_wf_data_pos += self._fill_dac_wf_buff(dac_wf_buff, dac_wf_data, dac_wf_data_pos)

    def read_adc_wfs(self) -> List[List[int]]:
        return self.adc_wfs


def run(epics_base_dir: Path, ioc_dir: Path, arch: str) -> None:

    prefix = epics_base_dir / "bin" / arch
    ioc = make_ioc(ioc_dir, arch)
    handler = Handler()

    scan_period = 1.0
    with FakeDev(prefix, ioc, handler):
        time.sleep(scan_period)

        handler.dac_wf_size = int(ca.get(prefix, "aao0.NELM"))
        logging.info(f"AAO SIZE = {handler.dac_wf_size}")
        dac_wf = []
        dac_wf.append([i for i in range(handler.dac_wf_size)])
        dac_wf.append([i for i in range(handler.dac_wf_size, 0, -1)])
        dac_wf.append([5 for i in range(handler.dac_wf_size // 2)])

        time.sleep(scan_period)

        #============
        dac_waveform_sleep = 1.5

        ca.put_array(prefix, "aao0", dac_wf[0])

        time.sleep(dac_waveform_sleep)

        assert len(handler.dac_wfs) == 1
        assert handler.dac_wfs[len(handler.dac_wfs) - 1] == dac_wf[0]

        ca.put_array(prefix, "aao0", dac_wf[1])
        ca.put_array(prefix, "aao0", dac_wf[2])

        time.sleep(dac_waveform_sleep * 2)

        assert len(handler.dac_wfs) == 3
        assert handler.dac_wfs[len(handler.dac_wfs) - 2] == dac_wf[1]
        assert handler.dac_wfs[len(handler.dac_wfs) - 1] == dac_wf[2]

        time.sleep(dac_waveform_sleep)

        assert len(handler.dac_wfs) == 3

        #=============

        time.sleep(scan_period)

        #=============
        handler.adc_wf_size = int(ca.get(prefix, "aai0.NELM"))
        adc_waveform_sleep = FakeDev.poll_ms_timeout / 1000 * (handler.adc_wf_size / FakeDev.adc_wf_msg_max_elems)
        adc_waveform_sleep = ceil(adc_waveform_sleep)

        for i in range(handler.adc_count):
            handler.adc_wfs[i] = [x for x in range(handler.adc_wf_size * 2)]

        adc_wf_numb = 0

        time.sleep(adc_waveform_sleep)

        adc_wf: List[List[int]] = [[] for i in range(handler.adc_count)]
        for i in range(handler.adc_count):
            logging.debug("aai%d:" % i)
            adc_wf[i] = [int(x) for x in ca.get_array(prefix, "aai%d" % i)]
            begin = adc_wf_numb * handler.adc_wf_size
            end = (adc_wf_numb + 1) * handler.adc_wf_size
            assert handler.adc_wfs[i][begin:end] == adc_wf[i]

        adc_wf_numb += 1

        time.sleep(adc_waveform_sleep)

        for i in range(handler.adc_count):
            logging.debug("aai%d:" % i)
            adc_wf[i] = [int(x) for x in ca.get_array(prefix, "aai%d" % i)]
            begin = adc_wf_numb * handler.adc_wf_size
            end = (adc_wf_numb + 1) * handler.adc_wf_size
            assert handler.adc_wfs[i][begin:end] == adc_wf[i]

    logging.info("Test passed!")
