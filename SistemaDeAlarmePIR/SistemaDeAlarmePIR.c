#include <stdio.h>
#include "pico/stdlib.h"

#define PIR 18
#define RED 13

int main()
{
    stdio_init_all();
    gpio_init(PIR);
    gpio_set_dir(PIR, GPIO_IN);
    
    gpio_init(RED);
    gpio_set_dir(RED, GPIO_OUT);
    gpio_put(RED, 0);

    while (true) {
        if(gpio_get(PIR) == 1){
            gpio_put(RED, 1);
            sleep_ms(10);

        }else{
            gpio_put(RED, 0);
            sleep_ms(10);

        }
        sleep_ms(10);
    }
}
