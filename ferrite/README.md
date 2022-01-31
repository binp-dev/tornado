# Ferrite

Framework for embedded heterogenous devices software development.

## About

### Use cases

+ Software development for specific hardware:
  + Real-time (code for MCU: bare-metal or with FreeRTOS).
  + Application (general-purpose code to run under Linux).
  + Control system interface to hardware (EPICS IOC (DeviceSupport)).
+ Establishing a communication between previous components.
+ Automation of building, testing and deployment processes.

### Supported platforms

+ NXP/Freescale i.MX7 *(deprecated)*
+ NXP/Freescale i.MX8M Nano

### Supported control systems

+ [EPICS](https://epics-controls.org/)

## Usage

To use the framework you need:

1. Include Ferrite to your project as submodule.
2. Specify your project components derived from Ferrite component templates.
3. Add `manage` script to your project.

You may look at Tornado project for reference usage examples.

Framework also provides some libraries with common routines:

+ C - for real-time code (including HAL).
+ C++ - for application code and IOC.
+ Python - for processes automation and CI integration.

## Framework testing

The framework contains tests for common use cases along with libraries unit tests. 

### Requirements

#### Linux packages

+ `g++`
+ `cmake`
+ `python3`
+ `perl`

#### Python packages

+ `poetry`

### Prepare

At first you need to install python dependencies. Run the following command in the project root:

```bash
poetry install
```

### Run tests

This command will build software and run all tests:

```bash
poetry run python -m ferrite.manage all.test
```
