#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_attr.h"
#include "soc/gpio_num.h"

#if defined(CONFIG_IDF_TARGET_ESP32C3)
    #define READ_PIN(pin) ((GPIO.in.val >> pin) & 1)
#elif defined(CONFIG_IDF_TARGET_ESP32)
    #define READ_PIN(pin) ((GPIO.in >> pin) & 1)
#else
    #define READ_PIN(pin) gpio_get_level(pin)
#endif

// Rotary knob encoder with event queue. Button is optional and debounced if used.
class RotaryButtonEncoder
{
    static constexpr uint16_t BUTTON_DEBOUNCE_MS = 10;
    static constexpr uint16_t EVENT_QUEUE_SIZE = 50;

public:
    // Creates a new rotary encoder. Use `begin()` to configure and attach to pins.
    RotaryButtonEncoder();
    ~RotaryButtonEncoder();
    // Setup with the specified pins. `pin_switch` is optional.
    void begin(gpio_num_t pin_clk, gpio_num_t pin_dt, gpio_num_t pin_switch = GPIO_NUM_NC);

    QueueHandle_t rotary_queue;

    enum class QueueEvent : uint8_t
    {
        NONE,
        ROTARY_LEFT,
        ROTARY_RIGHT,
        BUTTON_PRESS
    };

private:
    volatile gpio_num_t pin_sw;
    volatile gpio_num_t pin_clk;
    volatile gpio_num_t pin_dt;

    volatile uint8_t rotary_state;
    volatile uint8_t last_rotary_state;

    const QueueEvent quadTable[16] = {
        QueueEvent::NONE, QueueEvent::ROTARY_LEFT, QueueEvent::ROTARY_RIGHT, QueueEvent::NONE, // last_state = 00
        QueueEvent::ROTARY_RIGHT, QueueEvent::NONE, QueueEvent::NONE, QueueEvent::ROTARY_LEFT, // last_state = 01
        QueueEvent::ROTARY_LEFT, QueueEvent::NONE, QueueEvent::NONE, QueueEvent::ROTARY_RIGHT, // last_state = 10
        QueueEvent::NONE, QueueEvent::ROTARY_RIGHT, QueueEvent::ROTARY_LEFT, QueueEvent::NONE  // last_state = 11
    };

    TimerHandle_t button_debounce_timer;

    static void IRAM_ATTR rotaryISR(void *arg);
    static void IRAM_ATTR buttonISR(void *arg);

    static void debounceTimerCallbackStatic(TimerHandle_t x_timer);
    void debounceTimerCallback();
};