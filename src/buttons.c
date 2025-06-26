#include "buttons.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(buttons, LOG_LEVEL_INF);

static struct button_cfg buttons[] = {
    BUTTON_ENTRY(DT_ALIAS(up_slow),        UP_SLOW),
    BUTTON_ENTRY(DT_ALIAS(up_fast),        UP_FAST),
    BUTTON_ENTRY(DT_ALIAS(down_slow),      DOWN_SLOW),
    BUTTON_ENTRY(DT_ALIAS(down_fast),      DOWN_FAST),
    BUTTON_ENTRY(DT_ALIAS(brigue_slow),    BRIGUE_SLOW),
    /*BUTTON_ENTRY(DT_ALIAS(brigue_fast),    BRIGUE_FAST),
    BUTTON_ENTRY(DT_ALIAS(sion_slow),      SION_SLOW),
    BUTTON_ENTRY(DT_ALIAS(sion_fast),      SION_FAST),
    BUTTON_ENTRY(DT_ALIAS(sierre_slow),    SIERRE_SLOW),
    BUTTON_ENTRY(DT_ALIAS(sierre_fast),    SIERRE_FAST),
    BUTTON_ENTRY(DT_ALIAS(rhone_slow),     RHONE_SLOW),
    BUTTON_ENTRY(DT_ALIAS(rhone_fast),     RHONE_FAST),
    BUTTON_ENTRY(DT_ALIAS(vacuum_0),       VACUUM_0),
    BUTTON_ENTRY(DT_ALIAS(vacuum_1),       VACUUM_1),
    BUTTON_ENTRY(DT_ALIAS(emergency_stop), EMERGENCY_STOP),*/
};

static void (*button_state_callback)(uint16_t state) = NULL;
static uint16_t buttons_state_mask;

static void button_callback_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    struct button_cfg *btn = CONTAINER_OF(cb, struct button_cfg, gpio_cb);

    int val = gpio_pin_get(btn->gpio.port, btn->gpio.pin);
    bool pressed = (val == 0);  // active low

    if (pressed) {
        buttons_state_mask |= BIT(btn->index);
    } else {
        buttons_state_mask &= ~BIT(btn->index);
    }

    LOG_INF("Button %d %s (state=0x%04x)", btn->index, pressed ? "pressed" : "released", buttons_state_mask);

    if (button_state_callback) {
        button_state_callback(buttons_state_mask);
    }
}

int buttons_init(void (*state_cb)(uint16_t state))
{
    int ret;

    button_state_callback = state_cb;

    for (int i = 0; i < ARRAY_SIZE(buttons); i++) {
        if (!device_is_ready(buttons[i].gpio.port)) {
            LOG_ERR("GPIO port not ready for button %d", i);
            return -ENODEV;
        }

        ret = gpio_pin_configure_dt(&buttons[i].gpio, GPIO_INPUT);
        if (ret < 0) {
            LOG_ERR("Failed to configure button %d", i);
            return ret;
        }

        ret = gpio_pin_interrupt_configure_dt(&buttons[i].gpio, GPIO_INT_EDGE_BOTH);
        if (ret < 0) {
            LOG_ERR("Failed to set interrupt on button %d", i);
            return ret;
        }

        gpio_init_callback(&buttons[i].gpio_cb, button_callback_handler, BIT(buttons[i].gpio.pin));
        gpio_add_callback(buttons[i].gpio.port, &buttons[i].gpio_cb);
    }

    LOG_INF("Buttons initialized");
    return 0;
}