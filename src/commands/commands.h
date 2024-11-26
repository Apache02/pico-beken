#pragma once

#include "shell/Console.h"


void command_reset_spi(Console &c);

void command_chip_id(Console &c);

void command_xfer(Console &c);

void command_spi_read(Console &c);

void command_flash_bootloader(Console &c);

void command_dump(Console &c);

void command_erase_chip(Console &c);
