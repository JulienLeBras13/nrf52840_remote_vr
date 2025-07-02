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
#define NB_BUTTONS 15                       /// Number of buttons
#define DEBOUNCE_MS 10                      /// Debounce time
#define NUM_ISR_BUTTONS 8                   /// Number of buttons handled via ISR
#define POLLING_INTERVAL_MS 100             /// Polling interval for buttons not handled by ISR

/// Macro to define a button entry using GPIO information from DeviceTree
#define BUTTON_ENTRY(alias, idx_enum) \
    { .gpio = GPIO_DT_SPEC_GET_OR(alias, gpios, {0}), .index = idx_enum }

/// Enum use to handle the index of all buttons via their logical roles
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

/// Configuration for a single button
struct button_cfg {
    const struct gpio_dt_spec gpio;     /// GPIO specification from the DeviceTree
    struct gpio_callback gpio_cb;       /// Callback structure for GPIO interrupt handling
    button_index_t index;               /// Logical index from the enum above
};

/// Context structure for polling-based button handling
struct polling_ctx {
    struct k_work_delayable work;       /// Zephyr work item for periodic polling
    bool running;                       /// Indicates whether the polling is active
    uint8_t master_index;               /// Index used for grouping or addressing 
                                        /// (use depends on implementation)
    uint8_t slave_index;                /// Same as above, purpose depends on system logic
};

/** Initializes all buttons (ISR-based or polling-based) and registers a callback 
 *  to receive button state changes.
 *  @param state_cb Callback function that receives the current button state bitmap
 * */
int buttons_init(void (*state_cb)(uint16_t state));

#endif // BUTTONS_H_