import os
import fcntl
import struct
import time

# Definición de IOCTL
SIMTEMP_IOC_MAGIC = ord('t')
SIMTEMP_IOC_SET_PERIOD = 0x40047401  	# _IOW('t', 1, int)
SIMTEMP_IOC_SET_THRESHOLD = 0x40047402  # _IOW('t', 2, int)

DEV_PATH = "/dev/simtemp"
SYSFS_PATH = "/sys/class/simtemp_class/simtemp"

def sysfs_write(attr, value):
    with open(f"{SYSFS_PATH}/{attr}", "w") as f:
        f.write(str(value))

def ioctl_set(fd, cmd, value):
    fcntl.ioctl(fd, cmd, struct.pack("<i", value))

def read_temp():
    with open(DEV_PATH, "r") as f:
        return f.read().strip()

if __name__ == "__main__":
    print("=== Simulador de temperatura ===")
    fd = os.open(DEV_PATH, os.O_RDWR)
    # Configuración vía sysfs
    sysfs_write("sampling_ms", 1000)
    sysfs_write("threshold_mC", 30000)
    # Lectura periódica
    for _ in range(5):
        print(read_temp())
        time.sleep(1)
        
    # Configuración vía ioctl
    ioctl_set(fd, SIMTEMP_IOC_SET_PERIOD, 10)
    ioctl_set(fd, SIMTEMP_IOC_SET_THRESHOLD, 20000)
    print("Configuración vía ioctl aplicada")
    # Lectura periódica 
    for _ in range(5):
        print(read_temp())
        time.sleep(1)

    os.close(fd)
