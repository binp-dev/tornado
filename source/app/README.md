# Workarounds

## `imx_rpmsg_tty` output

Default `imx_rpmsg_tty` driver writing to much logs? especially on large RPMSG payload.
To avoid high storage and CPU usage you may disable `rsyslog` and `journald`:

```bash
systemctl disable rsyslog
systemctl stop rsyslog

systemctl mask systemd-journald
systemctl stop systemd-journald
```
*This is really bad solution. Need to fix `imx_rpmsg_tty` driver as soon as possible.*
