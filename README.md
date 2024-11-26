# Beken Chip SPI Flasher

This firmware for the **RP2040** is designed specifically to restore the bootloader on the **BK7252** chip via SPI.
It functions as a terminal and is based on [pico-console](https://github.com/Apache02/pico-console).


## Building the Firmware

```shell
mkdir build
cd build
cmake ..``
make
```

To flash the firmware onto the Pico, you can use:
```shell
make flash
```


## Connection

| Test Point | BK7252 GPIO | Description                      |     | RP2040 GPIO |
|------------|-------------|----------------------------------|-----|-------------|
| TDO        | 23          | Pin 38, Flash Data Output (MISO) | --> | 12          |
| TDI        | 22          | Pin 39, Flash Data Input (MOSI)  | --> | 11          |
| TMS        | 21          | Pin 40, Flash chip enable (CS)   | --> | 10          |
| TCK        | 20          | Pin 41, Flash Clock (SCLK)       | --> | 9           |
| CEN        | -           | Pin 43, Chip enable              | --> | 17          |


## Using the Firmware

1. Connect the wires according to the table above.
2. Connect to the Pico as a serial port using:
```shell
tio -b 115200 /dev/ttyACM0
```
3. Type `reset` to switch to Flash SPI mode.
4. Then use the `flash_bootloader` command to restore the bootloader.

A bootloader dump can be found at https://github.com/Apache02/bk7252-cam/tree/main/bootloaders


## Other Commands

* `help` - Displays a list of available commands.
* `chip_id` - Prints the result of the chip ID command (expected value: `15701c00`).
* `dump 0x00000000` - Displays 256 bytes starting at address `0x00000000`.
* `erase_chip` - Erases the entire chip.
* `xfer <byte0> [byte1] ... [byteN]` - Sends specified bytes over SPI and returns the response.


## Useful Links
* https://github.com/pfeerick/BK7231_SPI_Flasher
* https://www.elektroda.com/rtvforum/topic3931424.html
