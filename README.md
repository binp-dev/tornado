# Tornado

Software for next-gen power supply controller developed by Tornado.

Powered by Ferrite framework.

## Requirements

### Linux packages

+ `g++`
+ `cmake`
+ `python3`
+ `perl`

### Python packages

+ `poetry`

Remaining dependencies are automatically managed by `poetry`, you don't need to install them manually.

## Deploy dependencies

+ `ssh`
+ `rsync`

## Usage

### Preparation

At first you need to install python dependencies. Run the following command in the project root:

```bash
poetry install
```

### Testing

This command will build software and run all tests:

```bash
poetry run python -m tornado.manage host.all.test
```

### Run on the device

To build, deploy and run both aplication and real-time code and run it on the i.MX8M Nano device:

```bash
poetry run python -m tornado.manage device.all.run --device <ip-addr>[:port]
```

Device should be accessible through SSH as `root` user without password prompt.

### More information

To get more information about `manage` scripts run:

```bash
poetry run python -m tornado.manage --help
```
