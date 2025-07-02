#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <dk_buttons_and_leds.h>
#include "buttons_service.h"
#include "buttons.h"

/// Advertising parameters: connectable, using device identity address
static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	(BT_LE_ADV_OPT_CONNECTABLE |
	 BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
	800, /* Min Advertising Interval 500ms (800*0.625ms) */
	801, /* Max Advertising Interval 500.625ms (801*0.625ms) */
	NULL /* Set to NULL for undirected advertising */
);

LOG_MODULE_REGISTER(nrf52840_remote_vr, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define CON_STATUS_LED DK_LED2
#define USER_LED DK_LED3

#define STACKSIZE 1024
#define PRIORITY 7

#define RUN_LED_BLINK_INTERVAL 1000
#define NOTIFY_INTERVAL         500

/// Global variable to hold the current button state
static uint16_t app_buttons_state;

/// Advertising data: BLE flags and complete device name
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),

};

/// Scan response data: include 128-bit UUID of the buttons service
static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_BUTTONS_SERVICE_VAL),
};

/// Callback used by the GATT service to get the current button state
static uint16_t app_buttons_cb(void)
{
	return app_buttons_state;
}

/// Struct holding the application callbacks to be registered with the button service
static struct buttons_service_cb app_callbacks = {
	.buttons_cb = app_buttons_cb,
};

/// Called when button state changes
static void on_buttons_changed(uint16_t buttons_state)
{
	buttons_service_send_buttons_state_notify(buttons_state);
	app_buttons_state = buttons_state;
}

/// Called upon successful BLE connection
static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	printk("Connected\n");

	dk_set_led_on(CON_STATUS_LED);
}

/// Called when the BLE connection is terminated
static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);

	dk_set_led_off(CON_STATUS_LED);
}

/// Register connection callbacks
struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
};

int main(void)
{
	int blink_status = 0;
	int err;

	LOG_INF("Starting Remote VR device. \n");

	/// Initialize LEDs
	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)\n", err);
		return -1;
	}

	/// Initialize buttons and set callback for state change
	err = buttons_init(on_buttons_changed);
	if (err) {
		printk("Button init failed (err %d)\n", err);
		return -1;
	}

	/// Enable Bluetooth stack
	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return -1;
	}
	
	/// Register connection callbacks
	err = bt_conn_cb_register(&connection_callbacks);
	if (err) {
		LOG_ERR("Bluetooth connection callback register failed (err %d)\n", err);
		return -1;
	}

	/// Initialize custom button service with callback
	err = buttons_service_init(&app_callbacks);
	if (err) {
		printk("Failed to init LBS (err:%d)\n", err);
		return -1;
	}

	LOG_INF("Callback present: %d", app_callbacks.buttons_cb != NULL);
	LOG_INF("Bluetooth initialized\n");

	/// Start BLE advertising
	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return -1;
	}

	LOG_INF("Advertising successfully started\n");

	/// Main loop: blink LED to indicate device is running
	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
