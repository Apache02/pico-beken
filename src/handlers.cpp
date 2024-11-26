#include "commands/commands.h"
#include <stdio.h>


static void help(Console &console);

extern const Console::Handler handlers[] = {
        {"help",             help},
        {"reset",            command_reset_spi},
        {"chip_id",          command_chip_id},
        {"xfer",             command_xfer},
        {"spi_read",         command_spi_read},
        {"flash_bootloader", command_flash_bootloader},
        {"dump",             command_dump},
        {"erase_chip",       command_erase_chip},
        // required at the end
        {nullptr,            nullptr},
};


void help(Console &console) {
    printf("Commands:\r\n");
    for (int i = 0;; i++) {
        if (!handlers[i].name || !handlers[i].handler) {
            break;
        }

        printf("  %s\r\n", handlers[i].name);
    }
}
