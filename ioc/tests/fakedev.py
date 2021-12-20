import os
import time
import zmq

from threading import Thread

from ipp import AppMsg, McuMsg

from utils.epics.ioc import Ioc
from utils.epics import ca


def assert_eq(a, b, eps=1e2):
    assert abs(a - b) < eps

def send_msg(socket, msg):
    any_msg = None
    for i, f in enumerate(McuMsg.variants):
        if f.type.is_instance(msg):
            any_msg = McuMsg(i, msg)
            break
    assert any_msg is not None
    socket.send(any_msg.store())

def recv_msg(socket):
    return AppMsg.load(socket.recv())

def run_test(
    epics_base_dir,
    ioc_dir,
    ipp_dir,
    arch,
):
    global IDS

    ioc = Ioc(
        os.path.join(ioc_dir, "bin", arch, "PSC"),
        os.path.join(ioc_dir, "iocBoot/iocPSC/st.cmd")
    )

    context = zmq.Context()
    socket = context.socket(zmq.PAIR)
    socket.bind("tcp://127.0.0.1:8321")

    prefix = os.path.join(epics_base_dir, "bin", arch)

    global done
    global value
    done = False
    value = 0

    max_val = 0xFFFFFF
    some_val = 0xABCDEF
    
    out_wf_size = 200
    out_wf = []
    out_wf.append([x for x in range(out_wf_size)])
    out_wf.append([x for x in range(out_wf_size, 0, -1)])
    out_wf_msg_size = 7

    global input_wf
    input_wf = [[]]
    input_wf_size = 200
    input_wf_pos = 0
    
    def worker():
        global done
        global value

        assert recv_msg(socket).variant.is_instance(AppMsg.Start)
        print("Received start signal")
        send_msg(socket, McuMsg.Debug("Hello from MCU!"))

        poller = zmq.Poller()
        poller.register(socket, zmq.POLLIN)

        out_wf_numb = 0
        out_wf_pos = 0
        global input_wf
        input_wf_pos = 0

        time.sleep(1.0)

        while not done:
            send_msg(socket, McuMsg.WfReq())

            evts = poller.poll(1)
            if len(evts) != 0:
                msg = AppMsg.load(socket.recv())
                
                if msg.variant.is_instance(AppMsg.WfData):
                    new_data = msg.variant.data
                    # print("NEW DATA: ", len(new_data), " ", new_data)
                    
                    if len(new_data) <= input_wf_size - input_wf_pos:
                        input_wf[len(input_wf) - 1] += new_data
                        input_wf_pos += len(new_data)

                        if input_wf_pos == input_wf_size:
                            input_wf_pos = 0
                            input_wf.append([])
                    else:
                        input_wf[len(input_wf) - 1] += new_data[:input_wf_size - input_wf_pos]
                        nonfiting_data = new_data[input_wf_size - input_wf_pos:]
                        input_wf.append(nonfiting_data)
                        input_wf_pos = len(nonfiting_data)
                else:
                    raise Exception("Unexpected message type")
            
            if out_wf_numb < len(out_wf):
                old_out_wf_numb = out_wf_numb
                elements_to_send = out_wf_msg_size
                if out_wf_size - out_wf_pos < out_wf_msg_size:
                    elements_to_send = out_wf_size - out_wf_pos

                out_data = out_wf[out_wf_numb][out_wf_pos : out_wf_pos + elements_to_send]
                out_wf_pos += elements_to_send

                if out_wf_pos == out_wf_size:
                    out_wf_pos = 0
                    out_wf_numb += 1
                    if elements_to_send < out_wf_msg_size and out_wf_numb < len(out_wf): 
                        out_data += out_wf[out_wf_numb][:out_wf_msg_size - elements_to_send]
                        out_wf_pos += out_wf_msg_size - elements_to_send
                
                send_msg(socket, McuMsg.WfData(out_data))
                if old_out_wf_numb != out_wf_numb:
                    time.sleep(2.0)

    thread = Thread(target=worker)
    thread.start()

    with ca.Repeater(prefix), ioc:        
        caput_wf = [x for x in range(input_wf_size)]
        ca.put(prefix, "aao0", caput_wf, array=True)

        time.sleep(1.2)
        result = ca.get(prefix, "aai0", array=True)
        result = list(map(int, result))
        assert result == out_wf[0]

        time.sleep(2.2)
        result = ca.get(prefix, "aai0", array=True)
        result = list(map(int, result))
        assert result == out_wf[1]

        last_full_input_wf = input_wf[len(input_wf) - 2]
        if len(input_wf[len(input_wf) - 1]) == input_wf_size:
            last_full_input_wf = input_wf[len(input_wf) - 1]

        assert last_full_input_wf == caput_wf

    done = True
    thread.join()

    print("Test passed!")
