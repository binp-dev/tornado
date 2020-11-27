from script.util.subproc import run

def _ssh_prefix(addr, user="root"):
    comps = addr.split(":")
    if len(comps) == 2:
        host, port = comps
    elif len(comps) == 1:
        host, port = addr, "22"
    else:
        raise Exception(f"Bad address format: '{addr}'")
    return ["ssh", "-p", port, f"{user}@{host}"]

"""
def store(addr, src, dst):
    devcmd = "cat > m4image.bin && mount /dev/mmcblk0p1 /mnt && mv m4image.bin /mnt && umount /mnt"
    hostcmd = "test -f {img} && cat {img} | ssh root@{} '{}'".format(
        self.device, devcmd, img=os.path.join(self.output, "release/m4image.bin")
    )
    run(["bash", "-c", hostcmd])
"""
