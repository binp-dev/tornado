# PSC

Software for the BINP next-gen power supply controller.

## Requirements

### Debian packages

+ `git`
+ `g++`
+ `cmake`
+ `make`
+ `python3`
+ `python3-pip`

### Python packages

```bash
pip3 install -r requirements.txt
```

### Testing dependencies

+ `libgtest-dev`
+ `libzmq3-dev`


## Usage

### Test locally

This command will build and test IOC locally with fake device:

```bash
python3 -m manage ioc.test_fakedev
```

### Run on the device

To build, deploy and run both aplication and real-time code and run it on the real device:

```bash
python3 -m manage all.run --device <ip-addr>[:port]
```

### More information

To get more information about `manage` scripts run:

```bash
python3 -m manage --help
```

To get the list of components:

```bash
python3 -m manage .
```

To get the list of tasks for selected component:

```bash
python3 -m manage <component>.
```
