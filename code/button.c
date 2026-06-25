/*
Four-Legged Walker software, file for button interrupt
Justin Nascimento (U42983905) and Alvin Yan ()
*/
#include "button.h"

QueueHandle_t button_evt_queue;     // Queue to register button input
bool buttonFlag = 0;              // Flag to indicate when the button was pressed

// Increment Button interrupt handler -- add to button queue on increment button press
static void IRAM_ATTR button_isr_handler(void* arg){
  uint32_t gpio_num = (uint32_t) arg;

  // Set the button flag
  buttonFlag = 1;

  // Sends the flag to both event handlers
  xQueueSendFromISR(button_evt_queue, &gpio_num, NULL);
}

void button_init(void)
{

    // Set I/O Configurations
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = GPIO_INPUT_PIN_SEL,
        .pull_up_en = 1,
    };
    gpio_config(&io_conf);

    // Install Hardware interrupt and handler to GPIO
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_INPUT_IO_1, button_isr_handler, (void *) GPIO_INPUT_IO_1);
    gpio_isr_handler_add(GPIO_INPUT_IO_2, button_isr_handler, (void *) GPIO_INPUT_IO_2);
}