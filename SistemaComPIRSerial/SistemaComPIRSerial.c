#include <stdio.h>
#include "pico/stdlib.h"

#define PIR 18

int main()
{
    stdio_init_all();
    gpio_init(PIR);
    gpio_set_dir(PIR, GPIO_IN);

    while (true) {
        int status = gpio_get(PIR);
        printf("Status: %d", status);
        printf("\n");
        sleep_ms(1000);
    }
}
