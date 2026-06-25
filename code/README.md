# Code Readme

## File Structure
There are four function files, named alphanumeric.c, button.c, servo.c, and timer_interupt.c, each with .h header files of the same name. alphanumeric.c and alphanumeric.h contain the global variables, definitions, libraries, and functions needed for the alphanumeric display to work. Moreover, servo.c and servo.h contain the global variables, definitions, libraries, and functions needed to drive the servo motors. Furthermore, button.c and button.h contain the global variables, definitions, libraries, and functions needed to set up the buttons, their hardware interrupts, and their GPIO pins. Finally, timer_interrupt.c and timer_interrupt.h contain the global variables, definitions, libraries, and functions needed to initialize the timers and their hardware interrupts. Together, all these funcitons are utilized in the main walker.c, which also initializes the RTOS tasks and contains all the other logic and variables needed to achieve the goals of the project. 

## AI Declaration
- ChatGPT was used sparingly for bug fixes and logic simplification.
- An example ChatGPT thread that we used: https://chatgpt.com/share/698bbdd5-5f78-8013-a5c6-dc1860215768 
