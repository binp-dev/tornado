# PSC

Software for the BINP next-gen power supply controller.

## Requirements

### Debian packages

+ `g++`
+ `cmake`
+ `python3`
+ `pipenv`
+ `perl`

### Python packages

Python dependencies are automatically managed by `pipenv`, you don't need to install them manually.

## Deploy dependencies

+ `ssh`
+ `rsync`

## Usage

### Preparation

At first you need to install python dependencies using `pipenv`:

```bash
pipenv install
```

### Testing

This command will build software and run all tests (unit, codegen, fakedev):

```bash
pipenv run python -m manage host_all.test
```

### Run on the device

To build, deploy and run both aplication and real-time code and run it on the i.MX8M Nano device:

```bash
pipenv run python -m manage imx8mn_all.run --device <ip-addr>[:port]
```

Device should be accessible through SSH as `root` user without password prompt.

### More information

To get more information about `manage` scripts run:

```bash
pipenv run python -m manage --help
```

To get the list of components:

```bash
pipenv run python -m manage .
```

To get the list of tasks for selected component:

```bash
pipenv run python -m manage <component>.
```
