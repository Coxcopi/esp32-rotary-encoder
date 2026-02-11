#include "RotaryButtonEncoder.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_struct.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"

RotaryButtonEncoder::RotaryButtonEncoder()
{
    rotary_queue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(QueueEvent));
}

void RotaryButtonEncoder::begin(gpio_num_t pin_clk, gpio_num_t pin_dt, gpio_num_t pin_switch)
{
    this->pin_clk = pin_clk;
    this->pin_dt = pin_dt;
    this->pin_sw = pin_switch;

    esp_err_t err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE("RotaryEncoder", "Failed to install ISR service: %d", err);
        return;
    }

    uint64_t gpio_mask = (1ULL << pin_clk) | (1ULL << pin_dt);

    if (pin_switch != GPIO_NUM_NC)
    {
        button_debounce_timer = xTimerCreate(
            "debounce",
            pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS),
            pdFALSE,
            this, // pointer to encoder is passed as timer id
            debounceTimerCallbackStatic);

        gpio_mask |= (1ULL << pin_sw);

        if (button_debounce_timer) {
            xTimerStart(button_debounce_timer, 0);
        }
    }

    gpio_config_t cfg = {};
    cfg.pin_bit_mask = gpio_mask;
    cfg.mode = GPIO_MODE_INPUT;
    cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&cfg);

    gpio_set_intr_type(pin_clk, GPIO_INTR_ANYEDGE);
    gpio_isr_handler_add(pin_clk, rotaryISR, this);
    gpio_set_intr_type(pin_dt, GPIO_INTR_ANYEDGE);
    gpio_isr_handler_add(pin_dt, rotaryISR, this);

    if (pin_switch != GPIO_NUM_NC)
    {
        gpio_set_intr_type(pin_switch, GPIO_INTR_NEGEDGE);
        gpio_isr_handler_add(pin_switch, buttonISR, this);
    }

    last_rotary_state = (READ_PIN(pin_clk) << 1) | READ_PIN(pin_dt);
}

RotaryButtonEncoder::~RotaryButtonEncoder()
{
    gpio_isr_handler_remove(pin_clk);
    gpio_isr_handler_remove(pin_dt);
    if (button_debounce_timer)
    {
        xTimerStop(button_debounce_timer, 0);
        xTimerDelete(button_debounce_timer, 0);
        gpio_isr_handler_remove(pin_sw);
    }
}

void IRAM_ATTR RotaryButtonEncoder::rotaryISR(void *arg)
{
    auto encoder = static_cast<RotaryButtonEncoder *>(arg);
    encoder->rotary_state = READ_PIN(encoder->pin_clk) << 1 | READ_PIN(encoder->pin_dt);
    uint8_t idx = (encoder->last_rotary_state << 2) | encoder->rotary_state;
    encoder->last_rotary_state = encoder->rotary_state;
    QueueEvent change = encoder->quadTable[idx];
    if (change != QueueEvent::NONE)
    {
        BaseType_t hpw = pdFALSE;
        xQueueSendToBackFromISR(encoder->rotary_queue, &change, &hpw);
        if (hpw == pdTRUE)
            portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR RotaryButtonEncoder::buttonISR(void *arg)
{
    auto encoder = static_cast<RotaryButtonEncoder *>(arg);
    BaseType_t hpHasWoken = pdFALSE;
    xTimerResetFromISR(encoder->button_debounce_timer, &hpHasWoken);
}

void RotaryButtonEncoder::debounceTimerCallbackStatic(TimerHandle_t timer)
{
    auto *encoder = static_cast<RotaryButtonEncoder *>(
        pvTimerGetTimerID(timer));
    encoder->debounceTimerCallback();
}

void RotaryButtonEncoder::debounceTimerCallback()
{
    if (READ_PIN(pin_sw) == 0)
    {
        QueueEvent e = QueueEvent::BUTTON_PRESS;
        xQueueSend(rotary_queue, &e, 0);
    }
}
