/**@file
 * @defgroup buttons_service Button Service API
 * @{
 * @brief API for the Buttons Service.
 */

#ifndef BT_BUTTONS_SERVICE_H_
#define BT_BUTTONS_SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

//#include <zephyr/types.h>

/** @brief BUTTONS_SERVICE Service UUID. */
#define BT_UUID_BUTTONS_SERVICE_VAL 																			\
	BT_UUID_128_ENCODE(0x00001523, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/** @brief Buttons Characteristic UUID. */
#define BT_UUID_BUTTONS_SERVICE_BUTTONS_VAL                                                                     \
	BT_UUID_128_ENCODE(0x00001524, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

#define BT_UUID_BUTTONS_SERVICE BT_UUID_DECLARE_128(BT_UUID_BUTTONS_SERVICE_VAL)
#define BT_UUID_BUTTONS_SERVICE_BUTTONS_CHARACTERISTIC BT_UUID_DECLARE_128(BT_UUID_BUTTONS_SERVICE_BUTTONS_VAL)

/** @brief Callback type for when the button state is pulled. */
typedef uint16_t (*buttons_cb_t)(void);

/** @brief Callback struct used by the Button Service. */
struct buttons_service_cb {
	/** Button read callback. */
	buttons_cb_t buttons_cb;
};

/** @brief Initialize the Button Service.
 *
 * This function registers application callback functions with the Button
 * Service
 *
 * @param[in] callbacks Struct containing pointers to callback functions
 *			used by the service. This pointer can be NULL
 *			if no callback functions are defined.
 *
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int buttons_service_init(struct buttons_service_cb *callbacks);

/** @brief Send the button state as notification.
 *
 * This function sends a binary state, typically the state of a
 * button, to all connected peers.
 *
 * @param[in] button_state The state of the button.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int buttons_service_send_buttons_state_notify(uint16_t button_state);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_BUTTONS_SERVICE_H_ */
