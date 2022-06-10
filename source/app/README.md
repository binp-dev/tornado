# Application

## RPMsg driver

To load driver:

```bash
modprobe imx_rpmsg_tty
```

Add `imx_rpmsg_tty` to `/etc/modules` if you need to load driver on boot.

### `imx_rpmsg_tty` output

Default `imx_rpmsg_tty` driver writing to much logs, especially for large RPMSG payload.
To avoid high storage and CPU usage you may disable `rsyslog` and `journald`:

```bash
systemctl disable rsyslog
systemctl stop rsyslog

systemctl mask systemd-journald
systemctl stop systemd-journald
```

*This is really bad solution. Need to fix `imx_rpmsg_tty` driver instead.*
