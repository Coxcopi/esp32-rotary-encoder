// This example creates an encoder attached to some pins
// and then constantly reads the event queue and prints events to serial.
//
// Note: This example is configured to run on an ESP32-C3 (Devkit V1).
//       To use another board, edit the platformio.ini file.

#include <Arduino.h>
#include "RotaryButtonEncoder.h"

RotaryButtonEncoder rotary = RotaryButtonEncoder();

void setup()
{
  rotary.begin(GPIO_NUM_7, GPIO_NUM_6, GPIO_NUM_5);
}

void loop()
{
  Serial.begin(9600);
  RotaryButtonEncoder::QueueEvent event;
  if (xQueueReceive(rotary.rotary_queue, &event, 0))
  {
    Serial.println(static_cast<uint8_t>(event));
  }
}