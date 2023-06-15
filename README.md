# Tornado

Software for next-gen power supply controller.

## Requirements

### Host

+ `git`
+ `gcc`
+ `cmake`
+ `perl`
+ `python3`
+ `python3-poetry`
+ [`rustup`](https://rustup.rs/)

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

### Deploy to device

To build both application and real-time code and deploy it to the i.MX8M Nano device:

```bash
poetry run python -m tornado.manage device.deploy --device <ip-addr>[:port]
```

Device should be accessible through SSH as `root` user without password prompt.

### More information

To get more information about `manage` scripts run:

```bash
poetry run python -m tornado.manage --help
```
