/*
Four-Legged Walker software, Justin Nascimento (U42983905) and Alvin Yan ()
*/

#include <stdio.h>
#include <string.h>
#include "driver/i2c.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gptimer.h"
#include "freertos/semphr.h"

#include "alphanumeric.h"
#include "timer_interrupt.h"
#include "servo.h"
#include "button.h"

// ----------------------------------------------------------------------------------------
// Global Variables

#define NUMBER_STARTING_STEPS 3

// Define an Enum to indicate which mode the walker is in
enum Mode{
    Walking,
    Still
};
enum Mode mode = Still;

// RTOS Variables
static SemaphoreHandle_t mode_mutex;            // Mutex to indicate the mode
static SemaphoreHandle_t step_mutex;            // Mutex to indicate how many steps are left

// Servo Variables
servo_t servos[2];
mcpwm_cmpr_handle_t cmp_left, cmp_right;

// Walker Variables
int timeToActivation = 10;                              // Time to activation in seconds
int numberSteps = NUMBER_STARTING_STEPS;                // How many steps will be taken each session
int numberStepsLeft = NUMBER_STARTING_STEPS;            // How many steps left until next idle


// ---------------------------------------------------------------------------------------
// RTOS Task Definitions

// Task 1: Timer task - Every second, either update the time until the bot starts to walk again, or indicate how many steps are left
static void timer_evt_task(void *arg){

int evt;
char charBuffer[5];  // Stores the timeToActivation in Hours and minutes as a string

// Variables to store the time left in idle mode
unsigned int numberSecs;
unsigned int numberMins;
unsigned int numberHours;

// Variable to store the phase of the gait
size_t phase = 0;

while (1) {
    // If the Queue has a task (meaning the timer went off), process the data accordingly
    if (xQueueReceive(timer_queue, &evt, portMAX_DELAY)) {
        // If the walker is still, perform the countdown
        if(mode == Still){
            // Decrease the time to activation
            timeToActivation--;

            // Print a debug message
            printf("Time to activation: %d seconds\n", timeToActivation);

            // Process the data from seconds to hours
            numberSecs = timeToActivation % 60;
            numberMins = (timeToActivation % 3600) / 60;
            numberHours = timeToActivation / 3600;

            // Put the data into the character buffer
            // Clear the character buffer
            memset(charBuffer, 0, sizeof(charBuffer));

            // Store the variable in counter to the display buffer
            snprintf(charBuffer, sizeof(charBuffer), "%02d%02d", numberMins % 100, numberSecs % 100);

            displayMessageToAlphanumericScreen((uint8_t *) charBuffer);

            // Once the timer is reached, change the mode
            if(timeToActivation == 0){
                // Reset Time to Activation
                timeToActivation = 10;

                // Change the mode
                xSemaphoreTake(mode_mutex, portMAX_DELAY);
                mode = Walking;
                xSemaphoreGive(mode_mutex);

                // Debug message that walking starts
                printf("0 seconds remaining, Walking Initiates\n");
            }
        }
        // If the walker is walking, take a step and decreace the number of steps
        else if(mode == Walking){
            // Debug Message
            printf("%d steps remaining\n", numberStepsLeft);

            // Display the number of steps left
            // Clear the character buffer
            memset(charBuffer, 0, sizeof(charBuffer));

            // Store the variable in counter to the display buffer
            snprintf(charBuffer, sizeof(charBuffer), "%04d", numberStepsLeft);

            displayMessageToAlphanumericScreen((uint8_t *) charBuffer);

            // Add servo motion
            servo_set_angle(&(servos[0]),  walk_gait[phase].left_angle);
            servo_set_angle(&(servos[1]), walk_gait[phase].right_angle);

            phase = (phase + 1) % walk_gait_len;

            // Updates the number of steps remaining
            xSemaphoreTake(step_mutex, portMAX_DELAY);
            numberStepsLeft--;
            xSemaphoreGive(step_mutex);

            // If no more steps remain, reset variables and change mode
            if(numberStepsLeft == 0){
                // Reset numberStepsLeft
                xSemaphoreTake(step_mutex, portMAX_DELAY);
                numberStepsLeft = numberSteps;
                xSemaphoreGive(step_mutex);

                // Change the mode
                xSemaphoreTake(mode_mutex, portMAX_DELAY);
                mode = Still;
                xSemaphoreGive(mode_mutex);

                // Display 0  steps
                // Clear the character buffer
                memset(charBuffer, 0, sizeof(charBuffer));

                // Store the variable in counter to the display buffer
                snprintf(charBuffer, sizeof(charBuffer), "%04d", 0);

                displayMessageToAlphanumericScreen((uint8_t *) charBuffer);

                // Debug message that walking ends
                printf("0 steps remaining, Idle Initiates\n");
            }
        }
    }
}
}

// Task 2: Update Step Settings task - uses Console I/O to update the number of steps taken
static void update_step_settings_task(void *arg){
// Variable for xQueueReceive
uint32_t io_num;

// Debounce variable
static TickType_t last_press = 0;
TickType_t now;

while(1){
    if (xQueueReceive(button_evt_queue, &io_num, portMAX_DELAY)) {
            // If the button Flag is high, change directions, if not, do nothing
            if(buttonFlag == 1){

                // Get current time for Debounce
                now = xTaskGetTickCount();

                // Debounce mechanism
                if ((now - last_press) > pdMS_TO_TICKS(DEBOUNCE_MS)) {
                    last_press = now;

                    // Disable flag
                    buttonFlag = 0;

                    // Change the number of steps for the interval
                    if (io_num == GPIO_INPUT_IO_1) {
                        numberSteps++;
                        printf("User incremented number of Steps to %d\n", numberSteps);
                    }
                    else if (io_num == GPIO_INPUT_IO_2) {
                        numberSteps--;
                        printf("User decremented number of Steps to %d\n", numberSteps);
                    }

                    // Bounds it at a 1 step minimum
                    if(numberSteps <= 0){
                        numberSteps = 1;
                    }

                    xSemaphoreTake(step_mutex, portMAX_DELAY);
                    numberStepsLeft = numberSteps;
                    xSemaphoreGive(step_mutex);
                }
            }
        }
    }
}


// ---------------------------------------------------------------------------------------

void app_main(void)
{
    // Initializes I2C communication with alphanumeric Display
    i2c_example_master_init();
    i2c_scanner();

    // Initialize the Servos
    servos[0] = (servo_t){ // sg92r servo
        .comparator   = cmp_left,
        .min_pulse_us = 600,    // calibrated
        .max_pulse_us = 2300,   // calibrated
        .min_degree   = -90,
        .max_degree   =  90,
        .current_angle = 0
    };

    servos[1] = (servo_t){ // sg90 servo
        .comparator   = cmp_right,
        .min_pulse_us = 650,    // calibrated
        .max_pulse_us = 2820,   // temporary value plz change
        .min_degree   = -90,
        .max_degree   =  90,
        .current_angle = 0
    };
    servo_init(&servos[0],&servos[1]);



    // Initialize Mutexes
    mode_mutex = xSemaphoreCreateMutex();
    step_mutex = xSemaphoreCreateMutex();

    // Initiates the timer queue
    timer_queue = xQueueCreate(10, sizeof(int));
    // Initializes the button press queue
    button_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // Initialize the timer
    timer_init();
    button_init();

    // Sets the servo to neutral angles
    servo_set_angle(&servos[0], 0);
    servo_set_angle(&servos[1], 0);

    // Spins the task
    xTaskCreate(timer_evt_task, "timer_evt_task", 2048, NULL, 5, NULL);
    xTaskCreate(update_step_settings_task, "update_step_settings_task", 2048, NULL, 5, NULL);

    printf("Everything initialized\n");
}