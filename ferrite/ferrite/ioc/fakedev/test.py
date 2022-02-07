from __future__ import annotations
from typing import List

import time
from pathlib import Path

from ferrite.utils.epics.ioc import make_ioc
import ferrite.utils.epics.ca as ca
from ferrite.ioc.fakedev.base import FakeDev
from math import ceil

def assert_eq(a: float, b: float, eps: float = 1e-3) -> None:
    if abs(a - b) > eps:
        raise AssertionError(f"abs({a} - {b}) < {eps}")


class Handler(FakeDev.Handler):

    def __init__(self) -> None:
        self.channels = [0.0, 1.0, -1.0, 3.1415, -10.0, 10.0]
        
        self.adc_count = 6
        self.adc_wf_size = 1000
        self.adc_wfs = [[] for i in range(self.adc_count)]

        self.dac_wf_size = 1000
        self.dac_wfs = []

    def write_dac(self, voltage: float) -> None:
        self.channels[0] = voltage

    def read_adcs(self) -> List[float]:
        return self.channels


def assert_synchronized(prefix: Path, handler: Handler) -> None:
    for i, channel in enumerate(handler.channels):
        assert_eq(ca.get(prefix, f"ai{i}"), channel)


def run(epics_base_dir: Path, ioc_dir: Path, arch: str) -> None:

    prefix = epics_base_dir / "bin" / arch
    ioc = make_ioc(ioc_dir, arch)
    handler = Handler()

    scan_period = 1.0
    with FakeDev(prefix, ioc, handler):
        time.sleep(scan_period)
        assert_synchronized(prefix, handler)

        dac_wf = []
        dac_wf.append([i for i in range(handler.dac_wf_size)])
        dac_wf.append([i for i in range(handler.dac_wf_size, 0, -1)])
        dac_wf.append([5 for i in range(int(handler.dac_wf_size/2))])

        some_val = 2.718
        ca.put(prefix, "ao0", some_val)
        
        time.sleep(scan_period)
        
        assert_eq(handler.channels[0], some_val)

        #============
        dac_waveform_sleep = 1.5
        
        ca.put(prefix, "aao0", dac_wf[0], array=True)

        time.sleep(dac_waveform_sleep)

        assert len(handler.dac_wfs) == 1
        assert handler.dac_wfs[len(handler.dac_wfs) - 1] == dac_wf[0] 

        ca.put(prefix, "aao0", dac_wf[1], array=True)
        ca.put(prefix, "aao0", dac_wf[2], array=True)

        time.sleep(dac_waveform_sleep*2)

        assert len(handler.dac_wfs) == 3
        assert handler.dac_wfs[len(handler.dac_wfs) - 2] == dac_wf[1]
        assert handler.dac_wfs[len(handler.dac_wfs) - 1] == dac_wf[2]

        time.sleep(dac_waveform_sleep)

        assert len(handler.dac_wfs) == 3

        #=============

        some_val = 1.618
        handler.channels[0] = some_val        
        time.sleep(scan_period)
        assert_eq(ca.get(prefix, f"ai0"), some_val)

        #=============
        adc_waveform_sleep = FakeDev.poll_ms_timeout/1000 * (handler.adc_wf_size / FakeDev.adc_wf_msg_max_elems)
        adc_waveform_sleep = float(int(ceil(adc_waveform_sleep)))

        for i in range(handler.adc_count):
            handler.adc_wfs[i] = [x for x in range(handler.adc_wf_size * 2)]

        adc_wf_numb = 0

        time.sleep(adc_waveform_sleep)

        adc_wf = [[] for i in range(handler.adc_count)]
        for i in range(handler.adc_count):
            print("aai%d:" % i)
            adc_wf[i] = ca.get(prefix, "aai%d" % i, array=True)
            assert handler.adc_wfs[i][adc_wf_numb*handler.adc_wf_size : (adc_wf_numb + 1)*handler.adc_wf_size] == adc_wf[i]
        
        adc_wf_numb += 1

        time.sleep(adc_waveform_sleep)

        for i in range(handler.adc_count):
            print("aai%d:" % i)
            adc_wf[i] = ca.get(prefix, "aai%d" % i, array=True)
            assert handler.adc_wfs[i][adc_wf_numb*handler.adc_wf_size : (adc_wf_numb + 1)*handler.adc_wf_size] == adc_wf[i]

    print("Test passed!")
