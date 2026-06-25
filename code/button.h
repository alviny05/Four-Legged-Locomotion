/*
Four-Legged Walker software, file for button interrupt
Justin Nascimento (U42983905) and Alvin Yan ()
*/
#ifndef BUTTON_H
#define BUTTON_H

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "freertos/semphr.h"

#define GPIO_INPUT_IO_1       4
#define GPIO_INPUT_IO_2       26  // A0
#define ESP_INTR_FLAG_DEFAULT 0
#define GPIO_INPUT_PIN_SEL ((1ULL << GPIO_INPUT_IO_1) | (1ULL << GPIO_INPUT_IO_2))

#define DEBOUNCE_MS 250

// Queues and Flags for interrupts
extern QueueHandle_t button_evt_queue;              // Queue to register button input
extern bool buttonFlag;                              // Variable to store a flag to determine if the button was pressed

// Function Prototypes
void button_init(void);

#endif