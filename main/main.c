/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

#include <math.h>
#include <stdlib.h>

const int Y_PIN = 27;
const int X_PIN = 26;
const int UART_ID = uart0;
const int BAUD_RATE = 115200;
const int DEAD_ZONE = 180; // Example dead zone threshold


QueueHandle_t xQueueAdc;

typedef struct adc {
    int axis;
    int val;
} adc_t;



// Function to write data to UART
void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF;

    uart_putc_raw(uart0, data.axis);
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, msb);
    uart_putc_raw(uart0, -1);
}

// Task to handle UART transmission
void uart_task(void *p) {
    adc_t data;

    while (1) {
        if (xQueueReceive(xQueueAdc, &data, portMAX_DELAY)) {
            // printf("\nfila --> axis %d - %d\n", data.axis, data.val);
            write_package(data);
        }
    }
}


// Common function for reading and filtering ADC
int read_and_filter_adc(int axis) {
    adc_select_input(axis);
    int raw = adc_read();

    int scaled_val = ((raw - 2047) / 8);

    if (abs(scaled_val) < DEAD_ZONE) {
        scaled_val = 0; // Apply deadzone
    }

    return scaled_val / 12;
}

// Task to read X-axis and put data to the queue
void x_task(void *p) {
    adc_gpio_init(X_PIN);
    adc_t data;
    data.axis = 0;


    while (1) {
        int val = read_and_filter_adc(0);
        data.val = val;
        // printf("\naxis 0 --> %d\n", val);
        xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(500)); // Example delay
    }
}

// Task to read Y-axis and put data to the queue
void y_task(void *p) {
    adc_gpio_init(Y_PIN);
    adc_t data;
    data.axis = 1;

    while (1) {
        int val = read_and_filter_adc(1);
        data.val = val;
        // printf("\naxis 1 --> %d\n", val);
        xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(500)); // Example delay
    }
}

int main() {
    stdio_init_all();
    adc_init();

    xQueueAdc = xQueueCreate(32, sizeof(adc_t));

    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);
    xTaskCreate(x_task,    "x_task",    4096, NULL, 1, NULL);
    xTaskCreate(y_task,    "y_task",    4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
