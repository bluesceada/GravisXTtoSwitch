#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

/**
 * Test code that decodes Gravis GrIP for XT Gamepad
 */

#define GPIO_INPUT_CLK 34
#define GPIO_INPUT_DATA 35
#define ESP_INTR_FLAG_DEFAULT 0

static xQueueHandle data_recv_queue = NULL;

typedef struct {
    int x;
    int y;
    int ltrig;
    int rtrig;
    int throttle;
    int hatx;
    int haty;
    int dpadx;
    int dpady;
    int buttons;
    char init_status;
} Gamepad;

Gamepad grip = {0,0,0,0,0,0,0,0,0,0,0x0};

/* Some of this function is adapted from Linux Kernel GrIP Gameport Driver
 * from Vojtech Pavlik, Copyright (c) 1998-2001, (GPLv2) */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    static unsigned int i = 0;
    static unsigned int buf = 0;
    static unsigned char u, v, w = 0; // buffer of last three signal/clk edges
    unsigned int data_s = gpio_get_level(GPIO_INPUT_DATA);
    unsigned int clk_s = gpio_get_level(GPIO_INPUT_CLK);

    u = (data_s << 1) | clk_s;

    // rising or falling edge on CLK-signal -> fill buffer
    // TODO: try to move to separate input that doesn't need if-condition
    if ((u ^ v) & 1)
    {
        // 32bit int as bitstream-buffer of data (20bit)
        buf = (buf << 1) | data_s;
        i++;
    }
    // This condition detects if we are at the end (timing wise) of a 
    // 20-bit field, i.e. between two 20-bit fields
    // --> if valid, queue the last 20 bits in the bitstream buffer to get decoded
    // TODO: try to find a better condition, this is kind of a hack...
    else if (
        // Last two detected changes/interrupts were on DATA-signal:
               (((u ^ v) & (v ^ w)) >> 1)
        // CLK was always 0 during the last two interrupts (any pin change):
               & ~(u | v | w) & 1
            )
    {
        // should always be the case, except the very first time
        if (i == 20)
        {
            xQueueSendFromISR(data_recv_queue, &buf, NULL);
        }
        // always reset buffer, because we know that a 20-bit block ended:
        buf = 0;
        i = 0;
    }
    w = v;
    v = u;
}

static void grip_decode_task(void *arg)
{
    for (;;)
    {
        uint32_t buf;
        if (xQueueReceive(data_recv_queue, &buf, portMAX_DELAY))
        {
            /* Adapted from Linux Kernel GrIP Gameport Driver
             * from Vojtech Pavlik, Copyright (c) 1998-2001, (GPLv2) */
            unsigned int crc = buf ^ (buf >> 7) ^ (buf >> 14);
            if (!((crc ^ (0x25cb9e70 >> ((crc >> 2) & 0x1c))) & 0xf))
            {
                // first received 2 bits of buffer (=buf>>18) are
                // the index of the data field of which we have 4;
                // the last 4 are the crc;
                // leaves us 14 payload bits
                unsigned int field = (buf >> 18);
                int d = buf >> 4;
                grip.init_status |= 1 << field; // set init status bit for this field

                switch(field) {
                    case 0:
                        grip.x = (d >> 2) & 0x3f;
                        grip.y = 63 - ((d >> 8) & 0x3f);
                        break;
                    case 1:
                        grip.ltrig = (d >> 2) & 0x3f;
                        grip.rtrig = (d >> 8) & 0x3f;
                        break;
                    case 2:
                        grip.throttle = (d >> 8) & 0x3f;
                        grip.hatx = ((d >> 1) & 1) - (d & 1);
                        grip.haty = ((d >> 2) & 1) - ((d >> 3) & 1);
                        grip.dpadx = ((d >> 5) & 1) - ((d >> 4) & 1);
                        grip.dpady = ((d >> 6) & 1) - ((d >> 7) & 1);
                        break;
                    case 3:
                        grip.buttons = d >> 3;
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

static void printf_debug_task(void *arg) {
    for (;;) {
        // If all fields are init/valid, print them every 50ms
        if (grip.init_status == 0xf)
        {
            printf("Joystick X: %d\n", grip.x);
            printf("Joystick Y: %d\n", grip.y);
            printf("Analog Trigger Left: %d\n", grip.ltrig);
            printf("Analog Trigger Right: %d\n", grip.rtrig);
            printf("DPad X: %d\n", grip.dpadx);
            printf("DPad Y: %d\n", grip.dpady);
            printf("Coolie Hat X: %d\n", grip.hatx);
            printf("Coolie Hat Y: %d\n", grip.haty);
            printf("Throttle Slider: %d\n", grip.throttle);
            printf("Joystick Buttons: ");
            for (uint8_t j = 0; j < 11; j++)
                printf("%d", (grip.buttons >> j) & 0x1);
            printf("\n ---------------- \n");
        }
        vTaskDelay(50 / portTICK_RATE_MS);
    }
}


void app_main(void)
{
    gpio_config_t io_conf = {};
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    io_conf.intr_type = GPIO_INTR_ANYEDGE; //GPIO_INTR_POSEDGE;
    io_conf.pin_bit_mask = ((1ULL << GPIO_INPUT_CLK) | (1ULL << GPIO_INPUT_DATA));
    io_conf.mode = GPIO_MODE_INPUT;
    gpio_config(&io_conf);
    //gpio_set_intr_type(GPIO_INPUT_DATA, GPIO_INTR_POSEDGE);

    data_recv_queue = xQueueCreate(10, sizeof(uint32_t));

    xTaskCreate(grip_decode_task, "grip_decode_task", 2048, NULL, 10, NULL);
    xTaskCreate(printf_debug_task, "printf_debug_task", 2048, NULL, 5, NULL);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // both pins same ISR, but might be changed later
    gpio_isr_handler_add(GPIO_INPUT_CLK, gpio_isr_handler, (void *)GPIO_INPUT_CLK);
    gpio_isr_handler_add(GPIO_INPUT_DATA, gpio_isr_handler, (void *)GPIO_INPUT_DATA);

    while (1)
    {
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}
