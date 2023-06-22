'''
    server
    Anyone-Copyright 2023 何宏波
'''
import socket
import psutil
import time
import json
import wmi
import pynvml
import ctypes


def get_cpu_usage():
    return psutil.cpu_percent(interval=1)


def get_memory_usage():
    memory_info = psutil.virtual_memory()
    return memory_info.used / memory_info.total

# disk_name: 如 A B C
def get_disk_usage(disk_name : str):
    this_disk = disk_name + ':\\'
    disk_info = psutil.disk_usage(this_disk)
    return 1 - disk_info.free / disk_info.total


def get_gpu_usage():
    pynvml.nvmlInit()
    handle = pynvml.nvmlDeviceGetHandleByIndex(0)
    utilization = pynvml.nvmlDeviceGetUtilizationRates(handle)
    pynvml.nvmlShutdown()
    return utilization.gpu


# ipv4 tcp
# 端口 11451
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as lfd:
    lfd.bind(("0.0.0.0", 11451))
    lfd.listen()
    cfd, client_addr = lfd.accept()
    print(client_addr, "CONNECTED!")

    with cfd:
        while True:
            cpu_usage   = get_cpu_usage()
            memory_usage= get_memory_usage()
            diska_usage = get_disk_usage('A')
            diskb_usage = get_disk_usage('B')
            diskc_usage = get_disk_usage('C')
            gpu_usage   = get_gpu_usage()

            data = {
                'cpu'   : int(cpu_usage),
                'memory': int(memory_usage * 100),
                'diska' : int(diska_usage * 100),
                'diskb' : int(diskb_usage * 100),
                'diskc' : int(diskc_usage * 100),
                'gpu'   : int(gpu_usage)
            }
            for shit in data:
                cfd.send(ctypes.c_char(data[shit]))
                time.sleep(0.1)
            time.sleep(1)
