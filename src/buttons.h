#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

/** @file buttons.h
 * @brief Module for handling buttons mapped on nRF52840 DONGLE PINS
 */

#define BUTTON_PRESSED 1
#define BUTTON_RELEASED 0
#define MAX_BUTTONS 15
#define DEBOUNCE_MS 10
#define NUM_ISR_BUTTONS 8
#define POLLING_INTERVAL_MS 100

#define BUTTON_ENTRY(alias, idx_enum) \
    { .gpio = GPIO_DT_SPEC_GET_OR(alias, gpios, {0}), .index = idx_enum }

typedef enum {
    UP_SLOW,
    UP_FAST,
    DOWN_SLOW,
    DOWN_FAST,
    BRIGUE_SLOW,
    BRIGUE_FAST,
    SION_SLOW,
    SION_FAST,
    SIERRE_SLOW,
    SIERRE_FAST,
    RHONE_SLOW,
    RHONE_FAST,
    VACUUM_0,
    VACUUM_1,
    EMERGENCY_STOP
} button_index_t;

struct button_cfg {
    const struct gpio_dt_spec gpio;   // Données GPIO issues du DeviceTree
    struct gpio_callback gpio_cb;     // Structure pour gérer les interruptions
    button_index_t index;             // Index du bouton (issu de l'enum)
};

struct polling_ctx {
    struct k_work_delayable work;
    bool running;
    uint8_t master_index;
    uint8_t slave_index;
};

// Initialisation of all buttons with ISR
int buttons_init(void (*state_cb)(uint16_t state));

#endif // BUTTONS_H_