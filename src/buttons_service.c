/** @file
 *  @brief Buttons Service
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "buttons_service.h"

LOG_MODULE_REGISTER(buttons_service, LOG_LEVEL_ERR);

static uint16_t buttons_state_ble;

static bool buttons_service_notify_enabled;
static struct buttons_service_cb btns_callback;

static void buttons_service_ccc_cfg_changed(const struct bt_gatt_attr *attr,
    uint16_t value)
{
	buttons_service_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
}

int buttons_service_init(struct buttons_service_cb *callbacks){
    if (callbacks) {
        btns_callback.buttons_cb = callbacks->buttons_cb;
    }

    return 0;
}

static ssize_t read_buttons(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	LOG_INF("read_buttons() called — conn %p", (void*)conn);
	// get a pointer to button_state which is passed in the BT_GATT_CHARACTERISTIC() and stored in attr->user_data
	//const char *value = attr->user_data;

	LOG_INF("Attribute read, handle: %u, conn: %p", attr->handle, (void *)conn);

	if (btns_callback.buttons_cb) {
		// Call the application callback function to update the get the current value of the button
		buttons_state_ble = btns_callback.buttons_cb();
		return bt_gatt_attr_read(conn, attr, buf, len, offset, &buttons_state_ble/*value*/, sizeof(buttons_state_ble)/*sizeof(*value)*/);
	}

	return 0;
}

BT_GATT_SERVICE_DEFINE(
	buttons_service_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_BUTTONS_SERVICE),
	BT_GATT_CHARACTERISTIC(BT_UUID_BUTTONS_SERVICE_BUTTONS_CHARACTERISTIC, 
		BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, 
		BT_GATT_PERM_READ, read_buttons, NULL,
		&buttons_state_ble),
	BT_GATT_CCC(buttons_service_ccc_cfg_changed,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);

int buttons_service_send_buttons_state_notify(uint16_t button_state){
    if (!buttons_service_notify_enabled) {
        return -EACCES;
    }

    return bt_gatt_notify(NULL, &buttons_service_svc.attrs[2], &button_state, sizeof(button_state));
}