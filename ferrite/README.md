# PSC

Software for the BINP next-gen power supply controller.

## Requirements

### Debian packages

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

This command will build software and run all tests (unit, codegen, fakedev):

```bash
poetry run python -m ferrite.manage host_all.test
```

### Run on the device

To build, deploy and run both aplication and real-time code and run it on the i.MX8M Nano device:

```bash
poetry run python -m ferrite.manage imx8mn_all.run --device <ip-addr>[:port]
```

Device should be accessible through SSH as `root` user without password prompt.

### More information

To get more information about `manage` scripts run:

```bash
poetry run python -m ferrite.manage --help
```

To get the list of components:

```bash
poetry run python -m ferrite.manage .
```

To get the list of tasks for selected component:

```bash
poetry run python -m ferrite.manage <component>.
```
