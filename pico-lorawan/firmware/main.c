/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * This example uses OTAA to join the LoRaWAN network and then sends the 
 * internal temperature sensors value up as an uplink message periodically 
 * and the first byte of any uplink messages received controls the boards
 * built-in LED.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "hardware/adc.h"
#include "hardware/gpio.h"

#include "pico/stdlib.h"
#include "pico/lorawan.h"
#include "tusb.h"

#ifdef PICO_DEFAULT_LED_PIN
#define LED_PIN PICO_DEFAULT_LED_PIN
#endif

// edit with LoRaWAN Node Region and OTAA settings 
#include "config.h"

#include "Gas_Sensor.h"

#define BUZZER_PIN 15
#define LED_PIN_ALARM 17


// pin configuration for SX1276 radio module
const struct lorawan_sx1276_settings sx1276_settings = {
    .spi = {
        .inst = PICO_DEFAULT_SPI_INSTANCE,
        .mosi = PICO_DEFAULT_SPI_TX_PIN,
        .miso = PICO_DEFAULT_SPI_RX_PIN,
        .sck  = PICO_DEFAULT_SPI_SCK_PIN,
        .nss  = 8
    },
    .reset = 9,
    .dio0  = 7,
    .dio1  = 10
};

// OTAA settings
const struct lorawan_otaa_settings otaa_settings = {
    .device_eui   = LORAWAN_DEVICE_EUI,
    .app_eui      = LORAWAN_APP_EUI,
    .app_key      = LORAWAN_APP_KEY,
    .channel_mask = LORAWAN_CHANNEL_MASK
};

// variables for receiving data
int receive_length = 0;
uint8_t receive_buffer[242];
uint8_t receive_port = 0;

int main( void )
{
    
	//internal_temperature_init();
    DEV_Module_Init(); // Intiliazed the gas sensor module --> NOTE: should be out of while loop
	
    // initialize stdio and wait for USB CDC connect
    stdio_init_all();
    
       while (!tud_cdc_connected()) {
        tight_loop_contents();
    }
    
    printf("Pico LoRaWAN - OTAA - Temperature + LED\n\n");

    // initialize the LED pin and internal temperature ADC
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_init(LED_PIN_ALARM);
    gpio_set_dir(LED_PIN_ALARM, GPIO_OUT);
    gpio_init(BUZZER_PIN); //initializing the pinout 
    gpio_set_dir(BUZZER_PIN, GPIO_OUT); 

    // uncomment next line to enable debug
    //lorawan_debug(true);

    // initialize the LoRaWAN stack
    printf("Initilizating LoRaWAN ... ");
    if (lorawan_init_otaa(&sx1276_settings, LORAWAN_REGION, &otaa_settings) < 0) {
        printf("failed!!!\n");
        while (1) {
            tight_loop_contents();
        }
    } else {
        printf("success!\n");
    }

    // Start the join process and wait
    printf("Joining LoRaWAN network ...");
    lorawan_join();

    while (!lorawan_is_joined()) {
        lorawan_process_timeout_ms(1000);
        printf(".");
    }
    printf(" joined successfully!\n");

    float Gas_pres;
    // loop forever
    while (1) {
    
        float CO, Gas_pres; // Floats to store the gas values (CO is not used)

        Gas_Sensor(&CO, &Gas_pres); // return value is passed to the address of the floats
	
        // if the gas level is above 0.9 then turn the buzzer/light on
        if (Gas_pres > 0.9){
            gpio_put(LED_PIN_ALARM, 1);
            gpio_put(BUZZER_PIN, 1); 
        }

	    // Print the gas value to be sent
        printf("sending values: | Gas reading: %f |\n\n", Gas_pres);

        // send the gas value as a float in an unconfirmed uplink message
        if (lorawan_send_unconfirmed(&Gas_pres, sizeof(Gas_pres), 2) < 0) {
            printf("failed!!!\n");
        } else {
            printf("Message sent!\n\n");
        }

        // wait for up to 5000 mili seconds for an event
        if (lorawan_process_timeout_ms(5000) == 0) {
            // check if a downlink message was received
            receive_length = lorawan_receive(receive_buffer, sizeof(receive_buffer), &receive_port);
            if (receive_length > -1) {
                printf("received a %d byte message on port %d: ", receive_length, receive_port);

                for (int i = 0; i < receive_length; i++) {
                    printf("%02x", receive_buffer[i]);
                }
                printf("\n");

                // the first byte of the received message controls the on board LED
                gpio_put(PICO_DEFAULT_LED_PIN, receive_buffer[0]);
            }
        }

        // Turn the buzzer/light off
        gpio_put(LED_PIN_ALARM, 0);
        gpio_put(BUZZER_PIN, 0);
    }

    return 0;
}

