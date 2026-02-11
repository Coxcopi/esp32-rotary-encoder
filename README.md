## ESP32 rotary encoder library
A shitty esp32 libary for rotary encoders such as the generic **KY-040** module based on *FreeRTOS/EspIdf*. Uses hardware interrupts for input detection and a queue for accessing knob change and button press events.

See `examples/` on how to use.