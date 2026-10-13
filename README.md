# Custom Linux LED Driver for Raspberry Pi 3

A Linux platform driver developed in C for controlling an LED through GPIO and a Device Tree node. This project was created as part of embedded Linux driver development tasks.

## Features

- Linux platform driver using the Device Tree.
- GPIO control through the kernel GPIO descriptor API.
- Sysfs interface to control the LED and configure blinking.
- Configurable blinking interval in seconds.
- LED label retrieved from Device Tree.
- Delayed work used for periodic blinking.
- Resource cleanup when the driver is removed.

## Hardware and Software

- Raspberry Pi 3 Model B
- Embedded Linux built with Buildroot
- Linux kernel
- C and Linux kernel module APIs
- Device Tree (DTS)
- ARM cross-compilation toolchain

## Device Tree Configuration

The Device Tree contains two LED configurations:

- **GPIO17:** standard LED configuration using `gpio-leds`.
- **GPIO27:** custom platform device using `compatible = "custom,my-led"`.

The custom device uses the label `task6_led`, which the driver exposes through sysfs.

## Building

The supplied Makefile builds the kernel module using the configured Linux kernel build directory and ARM cross-compiler.

```bash
make
```

Before building, ensure that `KDIR` and `CROSS_COMPILE` in the Makefile point to the appropriate kernel build directory and cross-compilation toolchain in your environment.

The build produces `my_led.ko`.

## Loading the Driver

After copying the module to the Raspberry Pi and ensuring that the matching Device Tree configuration is active, load the module as root:

```bash
insmod my_led.ko
dmesg | tail
```

The driver creates the following sysfs directory:

```text
/sys/class/my_led/my_led/
```

## Sysfs Interface

| Attribute | Description |
|---|---|
| `state` | Read or set the LED state (`0` = off, `1` = on) |
| `interval` | Set the blinking interval in seconds (`0` = blinking disabled) |
| `label` | Read the label obtained from Device Tree |

### Examples

Read the Device Tree label and current state:

```bash
cat /sys/class/my_led/my_led/label
cat /sys/class/my_led/my_led/state
```

Turn the LED on:

```bash
echo 1 > /sys/class/my_led/my_led/state
```

Configure blinking every 5 seconds:

```bash
echo 5 > /sys/class/my_led/my_led/interval
```

Disable blinking and turn the LED off:

```bash
echo 0 > /sys/class/my_led/my_led/interval
echo 0 > /sys/class/my_led/my_led/state
```

## Unloading

Remove the driver when it is no longer in use:

```bash
rmmod my_led
```

## Project Structure

- `my_led.c` — Linux platform driver implementation.
- `bcm2837-rpi-3-b.dts` — Device Tree configuration.
- `Makefile` — kernel module build instructions.
