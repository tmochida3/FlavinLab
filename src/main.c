#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <string.h> // Needed for strncmp

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/services/nus.h>

LOG_MODULE_REGISTER(ble_motor_control, LOG_LEVEL_INF);

/*
 * ============================================================================
 * H-Bridge Motor Control Section
 * ============================================================================
 */

#define GPIO0_NODE DT_NODELABEL(gpio0)

// Pin definitions
#define NSLEEP_PIN 28    // P0.28
#define IN1_PIN    29    // P0.29
#define IN2_PIN    30    // P0.30

static const struct device *gpio_dev;

int motor_pins_init(void)
{
    gpio_dev = DEVICE_DT_GET(GPIO0_NODE);
    if (!device_is_ready(gpio_dev)) {
        LOG_ERR("GPIO device not ready");
        return -ENODEV;
    }
    gpio_pin_configure(gpio_dev, NSLEEP_PIN, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure(gpio_dev, IN1_PIN, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure(gpio_dev, IN2_PIN, GPIO_OUTPUT_INACTIVE);
    LOG_INF("Motor pins initialized");
    return 0;
}

void motor_enable(void)
{
    gpio_pin_set(gpio_dev, NSLEEP_PIN, 1);
    k_busy_wait(50);
}

void motor_disable(void)
{
    gpio_pin_set(gpio_dev, NSLEEP_PIN, 0);
}

void motor_stop(void)
{
    gpio_pin_set(gpio_dev, IN1_PIN, 0);
    gpio_pin_set(gpio_dev, IN2_PIN, 0);
}

void motor_forward(void)
{
    gpio_pin_set(gpio_dev, IN1_PIN, 1);
    gpio_pin_set(gpio_dev, IN2_PIN, 0);
}


/*
 * ============================================================================
 * Bluetooth LE & NUS Section
 * ============================================================================
 */

static struct bt_conn *current_conn;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

// Data received callback for NUS
static void ble_data_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
    ARG_UNUSED(conn);
    LOG_INF("Received data: %.*s", len, data);

    if (len >= 2 && strncmp(data, "ON", 2) == 0) {
        LOG_INF("Motor ON command received");
        motor_enable();
        motor_forward();
    } else if (len >= 3 && strncmp(data, "OFF", 3) == 0) {
        LOG_INF("Motor OFF command received");
        motor_stop();
        motor_disable();
    }
}

static struct bt_nus_cb nus_cb = {
    .received = ble_data_received,
};

// Connection callbacks
static void connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
        LOG_ERR("Connection failed (err %u)", err);
        return;
    }
    LOG_INF("Connected to %s", addr);
    current_conn = bt_conn_ref(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Disconnected from %s (reason %u)", addr, reason);

    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

/*
 * ============================================================================
 * Main Application
 * ============================================================================
 */

int main(void)
{
    int err;
    LOG_INF("Vibration Motor BLE Controller Starting...");

    // 1. Initialize motor control pins
    err = motor_pins_init();
    if (err) {
        LOG_ERR("Failed to initialize motor pins. Halting.");
        return 0;
    }

    // 2. Enable the Bluetooth stack
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return 0;
    }

    // 3. Initialize the Nordic UART Service (NUS)
    err = bt_nus_init(&nus_cb);
    if (err) {
        LOG_ERR("Failed to init NUS (err %d)", err);
        return 0;
    }

    // 4. Start advertising
    err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return 0;
    }

    LOG_INF("Setup complete. Advertising as 'VibMotor'.");
    return 0;
}