#include <stdio.h>
#include "pico/stdlib.h"
#include "shell/Console.h"


#define LED_PIN PICO_DEFAULT_LED_PIN


extern const Console::Handler handlers[];
Console *console;


int main() {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    console = new Console(handlers);
    console->reset();
    console->start();

    for (;;) {
        console->update(getchar());
    }
}
