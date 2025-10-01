// /*
//  * nRF5340 H-Bridge Motor Control Test
//  * For use with nRF Connect SDK / Zephyr
//  */

// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/gpio.h>
// #include <zephyr/logging/log.h>

// LOG_MODULE_REGISTER(motor_test, LOG_LEVEL_INF);

// /* The devicetree node identifier for the "led0" alias. */
// #define GPIO0_NODE DT_NODELABEL(gpio0)

// // Pin definitions
// #define NSLEEP_PIN 28    // P0.28
// #define IN1_PIN 29       // P0.29
// #define IN2_PIN 30       // P0.30

// static const struct device *gpio_dev;

// /**
//  * Initialize GPIO pins for H-bridge control
//  */
// int motor_pins_init(void)
// {
//     // Get GPIO device
//     gpio_dev = DEVICE_DT_GET(GPIO0_NODE);
//     if (!device_is_ready(gpio_dev)) {
//         LOG_ERR("GPIO device not ready");
//         return -ENODEV;
//     }

//     // Configure nSLEEP pin as output (start disabled)
//     int ret = gpio_pin_configure(gpio_dev, NSLEEP_PIN, GPIO_OUTPUT_INACTIVE);
//     if (ret < 0) {
//         LOG_ERR("Failed to configure nSLEEP pin: %d", ret);
//         return ret;
//     }

//     // Configure IN1 pin as output
//     ret = gpio_pin_configure(gpio_dev, IN1_PIN, GPIO_OUTPUT_INACTIVE);
//     if (ret < 0) {
//         LOG_ERR("Failed to configure IN1 pin: %d", ret);
//         return ret;
//     }

//     // Configure IN2 pin as output
//     ret = gpio_pin_configure(gpio_dev, IN2_PIN, GPIO_OUTPUT_INACTIVE);
//     if (ret < 0) {
//         LOG_ERR("Failed to configure IN2 pin: %d", ret);
//         return ret;
//     }

//     LOG_INF("Motor pins initialized: nSLEEP=P0.%d, IN1=P0.%d, IN2=P0.%d", 
//             NSLEEP_PIN, IN1_PIN, IN2_PIN);
//     return 0;
// }

// /**
//  * Enable the H-bridge chip
//  */
// void motor_enable(void)
// {
//     gpio_pin_set(gpio_dev, NSLEEP_PIN, 1);
//     k_busy_wait(50);  // Wait 50us for wake-up
//     LOG_DBG("Motor enabled");
// }

// /**
//  * Disable the H-bridge chip  
//  */
// void motor_disable(void)
// {
//     gpio_pin_set(gpio_dev, NSLEEP_PIN, 0);
//     LOG_DBG("Motor disabled");
// }

// /**
//  * Stop motor (coast mode - both inputs low)
//  */
// void motor_stop(void)
// {
//     gpio_pin_set(gpio_dev, IN1_PIN, 0);
//     gpio_pin_set(gpio_dev, IN2_PIN, 0);
//     LOG_DBG("Motor stopped");
// }

// /**
//  * Run motor forward (IN1=HIGH, IN2=LOW)
//  */
// void motor_forward(void)
// {
//     gpio_pin_set(gpio_dev, IN1_PIN, 1);
//     gpio_pin_set(gpio_dev, IN2_PIN, 0);
//     LOG_DBG("Motor forward");
// }

// /**
//  * Run motor reverse (IN1=LOW, IN2=HIGH)
//  */
// void motor_reverse(void)
// {
//     gpio_pin_set(gpio_dev, IN1_PIN, 0);
//     gpio_pin_set(gpio_dev, IN2_PIN, 1);
//     LOG_DBG("Motor reverse");
// }

// /**
//  * Brake motor (both inputs high)
//  */
// void motor_brake(void)
// {
//     gpio_pin_set(gpio_dev, IN1_PIN, 1);
//     gpio_pin_set(gpio_dev, IN2_PIN, 1);
//     LOG_DBG("Motor brake");
// }

// /**
//  * Simple vibration test
//  */
// void vibration_test(void)
// {
//     LOG_INF("Starting vibration test");
    
//     motor_enable();
    
//     // Test sequence: 3 short pulses
//     for (int i = 0; i < 3; i++) {
//         LOG_INF("Pulse %d/3", i + 1);
//         motor_forward();
//         k_msleep(200);  // 200ms on
//         motor_stop();
//         if (i < 2) {  // Don't delay after last pulse
//             k_msleep(100);  // 100ms off
//         }
//     }
    
//     // One longer vibration
//     k_msleep(500);
//     LOG_INF("Long vibration");
//     motor_forward();
//     k_msleep(800);  // 800ms on
//     motor_stop();
    
//     motor_disable();
//     LOG_INF("Vibration test complete");
// }

// /**
//  * Main function
//  */
// int main(void)
// {
//     LOG_INF("nRF5340 H-Bridge Simple Test Starting");
//     LOG_INF("This will test your coin vibration motor");

//     // Initialize pins
//     int ret = motor_pins_init();
//     if (ret < 0) {
//         LOG_ERR("Failed to initialize motor pins: %d", ret);
//         return ret;
//     }

//     // Wait a bit before starting
//     k_msleep(2000);

//     // Main test loop
//     while (1) {
//         LOG_INF("=== Running motor test cycle ===");
        
//         // Run vibration test
//         vibration_test();
        
//         // Wait 5 seconds before next test
//         LOG_INF("Waiting 5 seconds before next test...");
//         k_msleep(5000);
//     }

//     return 0;
// }



/*
 * nRF5340 H-Bridge Motor Control Test
 * For use with nRF Connect SDK / Zephyr
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(motor_test, LOG_LEVEL_INF);

/* The devicetree node identifier for the "led0" alias. */
#define GPIO0_NODE DT_NODELABEL(gpio0)

// Pin definitions - using easily accessible pins from nRF5340 DK
#define NSLEEP_PIN 28    // P0.28 - easily accessible on the DK
#define IN1_PIN 29       // P0.29 - easily accessible on the DK  
#define IN2_PIN 30       // P0.30 - easily accessible on the DK

static const struct device *gpio_dev;

/**
 * Initialize GPIO pins for H-bridge control
 */
int motor_pins_init(void)
{
    // Get GPIO device
    gpio_dev = DEVICE_DT_GET(GPIO0_NODE);
    if (!device_is_ready(gpio_dev)) {
        LOG_ERR("GPIO device not ready");
        return -ENODEV;
    }

    // Configure nSLEEP pin as output (start disabled)
    int ret = gpio_pin_configure(gpio_dev, NSLEEP_PIN, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure nSLEEP pin: %d", ret);
        return ret;
    }

    // Configure IN1 pin as output
    ret = gpio_pin_configure(gpio_dev, IN1_PIN, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure IN1 pin: %d", ret);
        return ret;
    }

    // Configure IN2 pin as output
    ret = gpio_pin_configure(gpio_dev, IN2_PIN, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure IN2 pin: %d", ret);
        return ret;
    }

    LOG_INF("Motor pins initialized: nSLEEP=P0.%d, IN1=P0.%d, IN2=P0.%d", 
            NSLEEP_PIN, IN1_PIN, IN2_PIN);
    return 0;
}

/**
 * Enable the H-bridge chip
 */
void motor_enable(void)
{
    gpio_pin_set(gpio_dev, NSLEEP_PIN, 1);
    k_busy_wait(50);  // Wait 50us for wake-up
    LOG_DBG("Motor enabled");
}

/**
 * Disable the H-bridge chip  
 */
void motor_disable(void)
{
    gpio_pin_set(gpio_dev, NSLEEP_PIN, 0);
    LOG_DBG("Motor disabled");
}

/**
 * Stop motor (coast mode - both inputs low)
 */
void motor_stop(void)
{
    gpio_pin_set(gpio_dev, IN1_PIN, 0);
    gpio_pin_set(gpio_dev, IN2_PIN, 0);
    LOG_DBG("Motor stopped");
}

/**
 * Run motor forward (IN1=HIGH, IN2=LOW)
 */
void motor_forward(void)
{
    gpio_pin_set(gpio_dev, IN1_PIN, 1);
    gpio_pin_set(gpio_dev, IN2_PIN, 0);
    LOG_DBG("Motor forward");
}

/**
 * Run motor reverse (IN1=LOW, IN2=HIGH)
 */
void motor_reverse(void)
{
    gpio_pin_set(gpio_dev, IN1_PIN, 0);
    gpio_pin_set(gpio_dev, IN2_PIN, 1);
    LOG_DBG("Motor reverse");
}

/**
 * Brake motor (both inputs high)
 */
void motor_brake(void)
{
    gpio_pin_set(gpio_dev, IN1_PIN, 1);
    gpio_pin_set(gpio_dev, IN2_PIN, 1);
    LOG_DBG("Motor brake");
}

/**
 * Simple vibration test
 */
void vibration_test(void)
{
    LOG_INF("Starting vibration test");
    
    motor_enable();
    
    // Test sequence: 3 short pulses
    for (int i = 0; i < 3; i++) {
        LOG_INF("Pulse %d/3", i + 1);
        motor_forward();
        k_msleep(200);  // 200ms on
        motor_stop();
        if (i < 2) {  // Don't delay after last pulse
            k_msleep(100);  // 100ms off
        }
    }
    
    // One longer vibration
    k_msleep(500);
    LOG_INF("Long vibration");
    motor_forward();
    k_msleep(800);  // 800ms on
    motor_stop();
    
    motor_disable();
    LOG_INF("Vibration test complete");
}

/**
 * Main function
 */
int main(void)
{
    LOG_INF("nRF5340 H-Bridge Simple Test Starting");
    LOG_INF("This will test your coin vibration motor");

    // Initialize pins
    int ret = motor_pins_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize motor pins: %d", ret);
        return ret;
    }

    // Wait a bit before starting
    k_msleep(2000);

    // Main test loop
    while (1) {
        LOG_INF("=== Running motor test cycle ===");
        
        // Run vibration test
        vibration_test();
        
        // Wait 5 seconds before next test
        LOG_INF("Waiting 5 seconds before next test...");
        k_msleep(5000);
    }

    return 0;
}