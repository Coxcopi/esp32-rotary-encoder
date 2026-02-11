// This example reads from a rotary encoder using a freertos task.
//
// Note: This example is configured to run on an ESP32-C3 (Devkit V1).
//       To use another board, edit the platformio.ini file.

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "RotaryButtonEncoder.h"

RotaryButtonEncoder rotary = RotaryButtonEncoder();

// task that repeatedly reads from the queue
void rotaryQueueTask(void *arg)
{
  while (true)
  {
    RotaryButtonEncoder::QueueEvent event;
    // wait indefinitely for an event
    if (xQueueReceive(rotary.rotary_queue, &event, portMAX_DELAY) == pdTRUE)
    {
      // handle event
      ESP_LOGI("RotaryTask", "Event: %u", static_cast<uint8_t>(event));
    }
  }
}

extern "C" void app_main()
{
  gpio_num_t pin_clk = GPIO_NUM_7;
  gpio_num_t pin_dt = GPIO_NUM_6;
  gpio_num_t pin_switch = GPIO_NUM_5;

  rotary.begin(pin_clk, pin_dt, pin_switch);
  // or without the switch:
  // rotary.begin(pin_clk, pin_dt);

  xTaskCreate(
      rotaryQueueTask,
      "RotaryQueueTask",
      4096,
      nullptr,
      10,
      nullptr);
}
