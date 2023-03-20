# Tornado

Software for next-gen power supply controller.

## Requirements

### Host

+ `git`
+ `gcc`
+ `cmake`
+ `perl`
+ `python3`
+ `poetry`
+ `rustup`

### Device

+ `ssh`
+ `rsync`

## Usage

### Preparation

Fetch submodules:

```bash
git submodule update --init
git submodule foreach 'git submodule update --init'
```

At first you need to install python dependencies. Run the following command in the project root:

```bash
poetry install
```

### Testing

This command will build software and run all tests:

```bash
poetry run python -m tornado.manage host.test
```

### Run on the device

To build, deploy and run both aplication and real-time code and run it on the i.MX8M Nano device:

```bash
poetry run python -m tornado.manage device.run --device <ip-addr>[:port]
```

Device should be accessible through SSH as `root` user without password prompt.

### More information

To get more information about `manage` scripts run:

```bash
poetry run python -m tornado.manage --help
```
