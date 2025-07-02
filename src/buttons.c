#include "buttons.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(buttons, LOG_LEVEL_INF);

/// Define all buttons using aliases from the DeviceTree and the enum indices 
static struct button_cfg buttons[] = {
    BUTTON_ENTRY(DT_ALIAS(up_slow),        UP_SLOW),
    BUTTON_ENTRY(DT_ALIAS(up_fast),        UP_FAST),
    BUTTON_ENTRY(DT_ALIAS(down_slow),      DOWN_SLOW),
    BUTTON_ENTRY(DT_ALIAS(down_fast),      DOWN_FAST),
    BUTTON_ENTRY(DT_ALIAS(brigue_slow),    BRIGUE_SLOW),
    BUTTON_ENTRY(DT_ALIAS(brigue_fast),    BRIGUE_FAST),
    BUTTON_ENTRY(DT_ALIAS(sion_slow),      SION_SLOW),
    BUTTON_ENTRY(DT_ALIAS(sion_fast),      SION_FAST),
    BUTTON_ENTRY(DT_ALIAS(sierre_slow),    SIERRE_SLOW),
    BUTTON_ENTRY(DT_ALIAS(sierre_fast),    SIERRE_FAST),
    BUTTON_ENTRY(DT_ALIAS(rhone_slow),     RHONE_SLOW),
    BUTTON_ENTRY(DT_ALIAS(rhone_fast),     RHONE_FAST),
    BUTTON_ENTRY(DT_ALIAS(vacuum_0),       VACUUM_0),
    BUTTON_ENTRY(DT_ALIAS(vacuum_1),       VACUUM_1),
    BUTTON_ENTRY(DT_ALIAS(emergency_stop), EMERGENCY_STOP),
};

/// Pointer to the application-level callback function for state changes
static void (*button_state_callback)(uint16_t state) = NULL;

/// Current and last state of all buttons (bitmask)
static uint16_t buttons_state_mask = 0;
static uint16_t last_buttons_state_mask = 0;

/// Array of contexts for polling-based buttons
static struct polling_ctx polling_tasks[NUM_ISR_BUTTONS];

/**
 * @brief Reads the physical state of a button (pressed or released)
 * 
 * @param index Index of the button in the array
 * @return BUTTON_PRESSED or BUTTON_RELEASED
 */
static int read_button_state(int index)
{
    int val = gpio_pin_get(buttons[index].gpio.port, buttons[index].gpio.pin);
    LOG_INF("Button index %d %s", index, val != 0 ? "pressed" : "relased");
    return val != 0 ? BUTTON_PRESSED : BUTTON_RELEASED;
}

/**
 * @brief Public function to read the state of a specific button
 * 
 * @param index Logical index of the button
 * @return State of the button or error code
 */
int button_read(button_index_t index)
{
    if (index >= NB_BUTTONS) {
        return -EINVAL;
    }

    return read_button_state(index);
}

/**
 * @brief Checks for changes in the button state and notifies via callback
 */
static void notify_state_change(void)
{
    if (buttons_state_mask != last_buttons_state_mask) {
        last_buttons_state_mask = buttons_state_mask;
        if (button_state_callback) {
            button_state_callback(buttons_state_mask);      /// Notify application
        }
    }
}

/**
 * @brief Polling work handler for buttons that require periodic sampling
 * 
 * This is used for "slave" buttons that are triggered by a corresponding "master" button.
 */
static void polling_work_handler(struct k_work *work)
{
    LOG_INF("Polling...");
    struct polling_ctx *ctx = CONTAINER_OF(work, struct polling_ctx, work.work);

    int master_state = read_button_state(ctx->master_index);
    int slave_state = read_button_state(ctx->slave_index);

    /// Update bitmask for slave button
    if (slave_state == BUTTON_PRESSED) {
        buttons_state_mask |= BIT(ctx->slave_index);
    } else {
        buttons_state_mask &= ~BIT(ctx->slave_index);
    }

    notify_state_change();

    /// Continue polling only if the master if still pressed
    if (master_state == BUTTON_PRESSED && ctx->running) {
        k_work_schedule(&ctx->work, K_MSEC(POLLING_INTERVAL_MS));
    } else {
        ctx->running = false;
    }
}

/**
 * @brief GPIO interrupt handler for button presses/releases
 * 
 * Handles state change and initiates polling for slave buttons if needed.
 */
static void button_callback_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    struct button_cfg *btn = CONTAINER_OF(cb, struct button_cfg, gpio_cb);

    int val = gpio_pin_get(btn->gpio.port, btn->gpio.pin);
    bool pressed = (val != 0);

    /// Update global state bitmask
    if (pressed) {
        buttons_state_mask |= BIT(btn->index);
    } else {
        buttons_state_mask &= ~BIT(btn->index);
    }

    LOG_INF("Button %d %s (state=0x%04x)", btn->index, pressed ? "pressed" : "released", buttons_state_mask);

    notify_state_change();

    /// For even-indexed buttons, start or stop polling of the associated slave
    if (btn->index % 2 == 0 && btn->index + 1 < NB_BUTTONS) {
        int ctx_idx = btn->index / 2;
        struct polling_ctx *ctx = &polling_tasks[ctx_idx];

        if (pressed) {
            ctx->master_index = btn->index;
            ctx->slave_index = btn->index + 1;
            ctx->running = true;
            k_work_schedule(&ctx->work, K_NO_WAIT);     /// Start polling immediately
        } else {
            ctx->running = false;                       /// Stop polling if master is released
            k_work_cancel_delayable(&ctx->work);
        }
    }
}


/**
 * @brief Initializes GPIOs, interrupts and polling for all buttons
 * 
 * @param state_cb Callback function to notify button state changes
 * @return 0 on success, negative error code on failure
 */
int buttons_init(void (*state_cb)(uint16_t state))
{
    int ret;
    button_state_callback = state_cb;

    /// Configure GPIOs and interrupts
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

        /// Configure interrupts only for even-indexed buttons (masters)
        if (i % 2 == 0) {
            ret = gpio_pin_interrupt_configure_dt(&buttons[i].gpio, GPIO_INT_EDGE_BOTH);
            if (ret < 0) {
                LOG_ERR("Failed to set interrupt on button %d", i);
                return ret;
            }

            gpio_init_callback(&buttons[i].gpio_cb, button_callback_handler, BIT(buttons[i].gpio.pin));
            gpio_add_callback(buttons[i].gpio.port, &buttons[i].gpio_cb);
        }
    }

    /// Initialize delayed work for polling contexts
    for (int i = 0; i < NUM_ISR_BUTTONS; i++) {
        k_work_init_delayable(&polling_tasks[i].work, polling_work_handler);
        polling_tasks[i].running = false;
    }

    LOG_INF("Buttons initialized");
    return 0;
}