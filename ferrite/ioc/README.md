# PSC-IOC

BINP next-gen power supply controller - Cortex-A7 Linux EPICS IOC

## Preparation

```bash
export EPICS_BASE=/path/to/epics-base
```

## Manage script

```bash
python3 -m manage <command> [options ...]
```

### Commands

| Command | Description |
|---------|-------------|
| `build` | Build release version                                 |
| `test`  | Build and perform tests (both unit and integration)   |
| `clean` | Remove all files created during test or release build |

### Options

To list all possible options:

```bash
python3 -m manage --help
```
