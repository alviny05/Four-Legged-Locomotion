/*
Four-Legged Walker software, file for alphanumeric display
Justin Nascimento (U42983905) and Alvin Yan ()
*/
#include "alphanumeric.h"


// ---------------------------------------------------------------------------------------
// Alphanumeric Display Functions
    // Functions for the Alphanumeric Display and I2C communication initialization
    // Function to initiate i2c -- note the MSB declaration!
    void i2c_example_master_init(void){
        // Debug
        printf("\n>> i2c Config\n");
        int err;
        int ret;

        // Port configuration
        int i2c_master_port = I2C_EXAMPLE_MASTER_NUM;

        /// Define I2C configurations
        i2c_config_t conf;
        conf.mode = I2C_MODE_MASTER;                              // Master mode
        conf.sda_io_num = I2C_EXAMPLE_MASTER_SDA_IO;              // Default SDA pin
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE;                  // Internal pullup
        conf.scl_io_num = I2C_EXAMPLE_MASTER_SCL_IO;              // Default SCL pin
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE;                  // Internal pullup
        conf.master.clk_speed = I2C_EXAMPLE_MASTER_FREQ_HZ;       // CLK frequency
        conf.clk_flags = 0;                                     // <-- UNCOMMENT IF YOU GET ERRORS (see readme.md)
        err = i2c_param_config(i2c_master_port, &conf);           // Configure
        if (err == ESP_OK) {printf("- parameters: ok\n");}

        // Install I2C driver
        err = i2c_driver_install(i2c_master_port, conf.mode,
                        I2C_EXAMPLE_MASTER_RX_BUF_DISABLE,
                        I2C_EXAMPLE_MASTER_TX_BUF_DISABLE, 0);
        // i2c_set_data_mode(i2c_master_port,I2C_DATA_MODE_LSB_FIRST,I2C_DATA_MODE_LSB_FIRST);
        if (err == ESP_OK) {printf("- initialized: yes\n\n");}

        // Dat in MSB mode
        i2c_set_data_mode(i2c_master_port, I2C_DATA_MODE_MSB_FIRST, I2C_DATA_MODE_MSB_FIRST);

        // Set up routines
        // Turn on alpha oscillator
        ret = alpha_oscillator();
        if(ret == ESP_OK) {printf("- oscillator: ok \n");}
        // Set display blink off
        ret = no_blink();
        if(ret == ESP_OK) {printf("- blink: off \n");}
        ret = set_brightness_max(0xF);
        if(ret == ESP_OK) {printf("- brightness: max \n");}
    }

    // Utility function to test for I2C device address -- not used in deploy
    int testConnection(uint8_t devAddr, int32_t timeout) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (devAddr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_stop(cmd);
        int err = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        return err;
    }

    // Utility function to scan for i2c device
    void i2c_scanner(void) {
        int32_t scanTimeout = 1000;
        printf("\n>> I2C scanning ..."  "\n");
        uint8_t count = 0;
        for (uint8_t i = 1; i < 127; i++) {
            // printf("0x%X%s",i,"\n");
            if (testConnection(i, scanTimeout) == ESP_OK) {
                printf( "- Device found at address: 0x%X%s", i, "\n");
                count++;
            }
        }
        if (count == 0)
            printf("- No I2C devices found!" "\n");
        printf("\n");
    }

    // Turn on oscillator for alpha display
    int alpha_oscillator(void) {
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, OSC, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    return ret;
    }

    // Set blink rate to off
    int no_blink(void) {
    int ret;
    i2c_cmd_handle_t cmd2 = i2c_cmd_link_create();
    i2c_master_start(cmd2);
    i2c_master_write_byte(cmd2, ( SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd2, HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON | (HT16K33_BLINK_OFF << 1), ACK_CHECK_EN);
    i2c_master_stop(cmd2);
    ret = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd2, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd2);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    return ret;
    }

    // Set Brightness
    int set_brightness_max(uint8_t val) {
    int ret;
    i2c_cmd_handle_t cmd3 = i2c_cmd_link_create();
    i2c_master_start(cmd3);
    i2c_master_write_byte(cmd3, ( SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd3, HT16K33_CMD_BRIGHTNESS | val, ACK_CHECK_EN);
    i2c_master_stop(cmd3);
    ret = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd3, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd3);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    return ret;
    }

    // Function to display a message to the alphanumeric display
    void displayMessageToAlphanumericScreen(uint8_t* messageBuffer){
        // Debug
        int ret;

        // I2C Command Handler
        i2c_cmd_handle_t cmd4;

        // Array to hold message in syntax screen requires
        uint16_t displaybuffer[16];

        // Converts characters from ASCII to syntax for the screen
        for(int i = 0; i < 4; i++){
            displaybuffer[i] = alphafonttable[messageBuffer[i]];
        }

        // Cycle through the characters to give "scrolling" effect
        // Send commands characters to display over I2C
        cmd4 = i2c_cmd_link_create();
        i2c_master_start(cmd4);
        i2c_master_write_byte(cmd4, ( SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
        i2c_master_write_byte(cmd4, (uint8_t)0x00, ACK_CHECK_EN);

        // Sends characters over I2C
        for (uint8_t i=0; i<4; i++) {
            i2c_master_write_byte(cmd4, displaybuffer[i] & 0xFF, ACK_CHECK_EN);
            i2c_master_write_byte(cmd4, displaybuffer[i] >> 8, ACK_CHECK_EN);
        }

        i2c_master_stop(cmd4);
        ret = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd4, 1000 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd4);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }