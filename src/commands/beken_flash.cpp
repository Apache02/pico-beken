#include "commands.h"

#include <stdio.h>
#include "pico/time.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"

#include "bootloader_crc.h"


#define BEKEN_FLASH_DO          (12)
#define BEKEN_FLASH_DI          (11)
#define BEKEN_FLASH_CLK         (10)
#define BEKEN_FLASH_CE          (9)
#define BEKEN_CEN               (17)

#define spi_instance            spi1
#define DEFAULT_BAUDRATE        (50'000)

#define FLASH_CHIP_ID           0x9F
#define FLASH_STATUS_WR_LOW     0x01
#define FLASH_STATUS_WR_HIGH    0x31
#define FLASH_WRITE_PAGE        0x02
#define FLASH_READ              0x03
#define FLASH_ENABLE_WRITE      0x06
#define FLASH_ERASE_SECTOR      0x20
#define FLASH_WAIT_ERASE        0x05
#define FLASH_ERASE_CHIP        0xc7


void spi_xfer(spi_inst_t *spi, const uint8_t *tx_buf, uint8_t *rx_buf, size_t length) {
    for (int i = 0; i < length; i++) {
        spi_read_blocking(spi, tx_buf[i], &rx_buf[i], 1);
    }
}

static void print_tx_(const uint8_t *buf, size_t length) {
    printf(COLOR_RED("TX") ": ");
    for (int i = 0; i < length; i++) {
        printf("%02X ", buf[i]);
    }
    printf("\r\n");
}

static void print_rx_(const uint8_t *buf, size_t length) {
    bool is_empty = true;
    printf(COLOR_GREEN("RX") ": ");
    for (int i = 0; i < length; i++) {
        printf("%02X ", buf[i]);
        if (buf[i] != 0) is_empty = false;
    }
    if (is_empty) {
        printf(COLOR_RED("(empty)"));
    } else {
        printf(COLOR_GREEN("!"));
    }
    printf("\r\n");
}

#define print_tx(buf, size)         print_tx_(buf, size)
#define print_rx(buf, size)         print_rx_(buf, size)


static void bk_reset(int delay = 1000) {
    // reset chip
    gpio_init(BEKEN_CEN);
    gpio_put(BEKEN_CEN, 0);
    gpio_set_dir(BEKEN_CEN, GPIO_OUT);
    sleep_ms(delay);
    gpio_put(BEKEN_CEN, 1);
}

static bool reset_to_spi_flash() {
    printf("Enabling SPI Flash .");

    // reset spi pins
    gpio_init(BEKEN_FLASH_DO);
    gpio_init(BEKEN_FLASH_DI);
    gpio_init(BEKEN_FLASH_CLK);
    gpio_init(BEKEN_FLASH_CE);

    spi_init(spi_instance, DEFAULT_BAUDRATE);
    spi_set_format(spi_instance, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(BEKEN_FLASH_DO, GPIO_FUNC_SPI);
    gpio_set_function(BEKEN_FLASH_DI, GPIO_FUNC_SPI);
    gpio_set_function(BEKEN_FLASH_CLK, GPIO_FUNC_SPI);

    for (int i = 0; i < 10; i++) {
        gpio_put(BEKEN_FLASH_CE, 1);
        gpio_set_dir(BEKEN_FLASH_CE, GPIO_OUT);

        uint8_t buf[250];
        memset(buf, 0xD2, count_of(buf));

        bk_reset(1000);
        gpio_put(BEKEN_FLASH_CE, 0);

        // skip exactly 16 bytes
        spi_write_blocking(spi_instance, buf, 16);
        print_tx(buf, 16);

        spi_read_blocking(spi_instance, 0xD2, buf, count_of(buf));
        print_rx(buf, sizeof(buf)); // buf should be [0xD2, 0, 0, ...]

        if (buf[0] == 0xD2 && buf[1] == 0 && buf[2] == 0 && buf[3] == 0) {
            printf(" " COLOR_GREEN("SUCCESS") "\n");

            // I don't know why is required
            spi_read_blocking(spi_instance, 0, buf, 32);
            print_rx(buf, 32);

            printf("\n");
            gpio_put(BEKEN_FLASH_CE, 1);

            return true;
        }

        putchar('.');
    }

    return false;
}

void flash_cmd_chip_id(uint8_t *data) {
    gpio_put(BEKEN_FLASH_CE, 0);

    uint8_t tx[4] = {FLASH_CHIP_ID, 0x00, 0x00, 0x00};
    uint8_t rx[4];

    spi_xfer(spi_instance, tx, rx, sizeof(tx));

    print_tx(tx, 4);
    print_rx(rx, 4);

    gpio_put(BEKEN_FLASH_CE, 1);

    if (data) {
        data[0] = rx[0];
        data[1] = rx[1];
        data[2] = rx[2];
        data[3] = rx[3];
    }
}

void command_reset_spi(Console &c) {
    printf("\n");
    printf("Pins: %d + %d + %d + %d | %d\n", BEKEN_FLASH_DO, BEKEN_FLASH_DI, BEKEN_FLASH_CLK, BEKEN_FLASH_CE,
           BEKEN_CEN);
    printf("Instance: %s\n", spi_instance == spi0 ? "spi0" : "spi1");
    printf("Baudrate: %d\n", DEFAULT_BAUDRATE);

    if (!reset_to_spi_flash()) {
        printf("Fail\n");
        return;
    }

    sleep_ms(5);

    uint32_t chip_id;
    flash_cmd_chip_id((uint8_t *) &chip_id);
    if (chip_id == 0xD2) {
        // try to repeat
        flash_cmd_chip_id((uint8_t *) &chip_id);
    }

    printf("Chip ID: %08lx\n", chip_id);
}

void command_chip_id(Console &c) {
    uint32_t chip_id;
    flash_cmd_chip_id((uint8_t *) &chip_id);
    printf("Chip ID: %08lx\n", chip_id);
}

void command_xfer(Console &c) {
    uint8_t tx[128];
    uint8_t rx[128];
    size_t length = 0;

    for (int i = 0; i < count_of(tx); i++) {
        auto result = c.packet.take_int();
        if (result.is_err()) {
            break;
        } else {
            tx[length++] = (int) result;
        }
    }

    if (length == 0) {
        printf(COLOR_RED("Arguments required") "\r");
        printf("Usage: %s <byte0> [byte1] ... [byteN] \n", c.packet.buf);

        return;
    }

    gpio_put(BEKEN_FLASH_CE, 0);
    spi_xfer(spi_instance, tx, rx, sizeof(tx));
    gpio_put(BEKEN_FLASH_CE, 1);

    print_tx(tx, length);
    print_rx(rx, length);

    for (int i = 0; i < length; i++) {
        printf("%02X ", rx[i]);
        if ((i & 0xF) == 0xF || i == (length - 1)) {
            printf("\n");
        }
    }
}

void command_spi_read(Console &c) {
    auto result = c.packet.take_int();
    if (result.is_err()) {
        printf(COLOR_RED("Argument required") "\n");
        return;
    }

    size_t count = (int) result;
    if (count <= 0) {
        printf(COLOR_RED("Invalid count: %d") "\n", count);
    }

    uint8_t *rx_buf = new uint8_t[count];

    gpio_put(BEKEN_FLASH_CE, 0);
    spi_read_blocking(spi_instance, 0, rx_buf, count * sizeof(rx_buf[0]));
    gpio_put(BEKEN_FLASH_CE, 1);

    print_rx(rx_buf, count * sizeof(rx_buf[0]));

    delete[] rx_buf;
}


void flash_cmd_enable_write() {
    printf("%s\n", __PRETTY_FUNCTION__);

    gpio_put(BEKEN_FLASH_CE, 0);
    uint8_t cmd[1] = {FLASH_ENABLE_WRITE};
    spi_write_blocking(spi_instance, cmd, 1);
    print_tx(cmd, 1);
    gpio_put(BEKEN_FLASH_CE, 1);
}

void flash_cmd_wait_busy_down() {
    printf("%s\n", __PRETTY_FUNCTION__);

    for (;;) {
        gpio_put(BEKEN_FLASH_CE, 0);
        uint8_t tx[2] = {FLASH_WAIT_ERASE, 0};
        uint8_t rx[2] = {0, 0};
        spi_xfer(spi_instance, tx, rx, sizeof(tx));
        gpio_put(BEKEN_FLASH_CE, 1);

        print_tx(tx, sizeof(tx));
        print_rx(rx, sizeof(rx));
        if ((rx[1] & 0x01) == 0) {
            break;
        }
    }
}

void flash_cmd_erase_sector(uint32_t addr) {
    printf("%s\n", __PRETTY_FUNCTION__);

    gpio_put(BEKEN_FLASH_CE, 0);
    uint8_t cmd[4] = {
            FLASH_ERASE_SECTOR,
            (uint8_t) ((addr >> 16) & 0xFF),
            (uint8_t) ((addr >> 8) & 0xFF),
            (uint8_t) ((addr >> 0) & 0xFF),
    };
    spi_write_blocking(spi_instance, cmd, sizeof(cmd));
    print_tx(cmd, sizeof(cmd));
    gpio_put(BEKEN_FLASH_CE, 1);

    flash_cmd_wait_busy_down();
}

void flash_cmd_erase_chip() {
    printf("%s\n", __PRETTY_FUNCTION__);

    gpio_put(BEKEN_FLASH_CE, 0);
    uint8_t cmd[1] = {FLASH_ERASE_CHIP};
    spi_write_blocking(spi_instance, cmd, sizeof(cmd));
    print_tx(cmd, sizeof(cmd));
    gpio_put(BEKEN_FLASH_CE, 1);

    flash_cmd_wait_busy_down();
}

void flash_cmd_write_256_bytes(uint32_t addr, const uint8_t *buf) {
    printf("%s\n", __PRETTY_FUNCTION__);

    gpio_put(BEKEN_FLASH_CE, 0);
    uint8_t cmd[4] = {
            FLASH_WRITE_PAGE,
            (uint8_t) ((addr >> 16) & 0xFF),
            (uint8_t) ((addr >> 8) & 0xFF),
            (uint8_t) ((addr >> 0) & 0xFF),
    };
    spi_write_blocking(spi_instance, cmd, sizeof(cmd));
    spi_write_blocking(spi_instance, buf, 256);

    print_tx(cmd, sizeof(cmd));
    print_tx(buf, 256);

    gpio_put(BEKEN_FLASH_CE, 1);

    flash_cmd_wait_busy_down();
}

void flash_cmd_status_write() {
    printf("%s\n", __PRETTY_FUNCTION__);

    gpio_put(BEKEN_FLASH_CE, 0);

    uint8_t cmd[0x40] = {FLASH_STATUS_WR_LOW, 0, 0};
    spi_write_blocking(spi_instance, cmd, sizeof(cmd));
    print_tx(cmd, sizeof(cmd));

    gpio_put(BEKEN_FLASH_CE, 1);

    flash_cmd_wait_busy_down();
}

void command_flash_bootloader(Console &c) {
    // bootloader start address
    uint32_t addr = 0x00000000;

    for (; addr < sizeof(bootloader_crc_bin);) {
        if ((addr & 0xFFF) == 0) {
            flash_cmd_enable_write();
            flash_cmd_erase_sector(addr);
        }

        flash_cmd_enable_write();
        flash_cmd_write_256_bytes(addr, ((uint8_t *) bootloader_crc_bin) + addr);
        addr += 256;
    }

    flash_cmd_enable_write();
    flash_cmd_status_write();

    printf("\ndone\n");
}

void command_dump(Console &c) {
    auto addr = c.packet.take_int().ok_or(0);
    printf("addr 0x%08x\r\n", addr);

    gpio_put(BEKEN_FLASH_CE, 0);

    uint8_t cmd[4] = {
            FLASH_READ,
            (uint8_t) ((addr >> 16) & 0xFF),
            (uint8_t) ((addr >> 8) & 0xFF),
            (uint8_t) ((addr >> 0) & 0xFF),
    };
    spi_write_blocking(spi_instance, cmd, sizeof(cmd));
    print_tx(cmd, sizeof(cmd));

    uint8_t rx[16 * 16];
    spi_read_blocking(spi_instance, 0, rx, sizeof(rx));

    gpio_put(BEKEN_FLASH_CE, 1);

    for (size_t i = 0; i < sizeof(rx);) {
        printf("%02x ", rx[i]);
        i += sizeof(uint8_t);
        if (i % 16 == 0) {
            printf("\r\n");
        }
    }
}

void command_erase_chip(Console &c) {
    flash_cmd_enable_write();
    flash_cmd_erase_chip();
    flash_cmd_enable_write();
    flash_cmd_status_write();

    printf("\nerase chip done\n");
}