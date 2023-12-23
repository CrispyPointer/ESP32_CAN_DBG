/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"

#include "driver/twai.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"

#define GPIO_OUTPUT_PIN_SEL                                                    \
  ((1ULL << 9u) | (1ULL << 8u) | (1ULL << 7u) | (1ULL << 6u))

static void can_receive_task(void *pvParameters) {
  while (1) {
    static uint32_t counter;
    static twai_message_t message;

    if (twai_receive(&message, 0) == ESP_OK) {
      gpio_set_level(9u, 0);
      printf("Counter value: %ld ", counter);
      printf("ID is %ld data:[", message.identifier);
      if (!(message.rtr)) {
        for (int i = 0; i < message.data_length_code; i++) {
          printf("%d,", message.data[i]);
        }
        printf("]\n");
      }
      counter += 1;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(9u, 1);
  }
}

static void can_transmit_task(void *Arg) {}

void app_main(void) {
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  gpio_config_t io_conf = {};
  // disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE;
  // set as output mode
  io_conf.mode = GPIO_MODE_OUTPUT;
  // bit mask of the pins that you want to set,e.g.GPIO18/19
  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  // disable pull-down mode
  io_conf.pull_down_en = 0;
  // disable pull-up mode
  io_conf.pull_up_en = 0;
  // configure GPIO with the given settings
  gpio_config(&io_conf);

  gpio_set_level(6u, 0);

  twai_general_config_t g_config =
      TWAI_GENERAL_CONFIG_DEFAULT(0u, 3u, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
  ESP_ERROR_CHECK(twai_start());
  twai_clear_receive_queue();

  // memset(message.data, 0x2u, 8u);
  // Configure message to transmit
  twai_message_t message;
  message.identifier = 0x2A0;
  message.data_length_code = 4;
  for (int i = 0; i < 4; i++) {
    message.data[i] = 0;
  }

  // Queue message for transmission
  if (twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK) {
    printf("Message queued for transmission\n");
  } else {
    printf("Failed to queue message for transmission\n");
  }

  // xTaskCreate(can_transmit_task, "can_tx_task", 1024*3, NULL, 8u, NULL);
  xTaskCreate(can_receive_task, "can_rx_task", 1024 * 3, NULL, 8u, NULL);
}
