# PSC

Software for the BINP next-gen power supply controller.

## Requirements

### Python packages

```bash
pip3 install -r script/requirements.txt
```

### Testing dependencies

+ `catch`
+ `libzmq3-dev`
+ `libczmq-dev`


## Usage

For example, to build, deploy and run tests:

```bash
python3 -m script test --dev-addr <device-ip-address>
```
