/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_sleep.h"
#include "unity.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "soc/rtc_io_periph.h"

#ifdef CONFIG_IDF_TARGET_ESP32
// The input-only rtcio pins do not have pull-up/down resistors (not support pull-up/down)
#define RTCIO_SUPPORT_PU_PD(num)    (rtc_io_desc[num].pullup != 0)
#define TEST_GPIO_PIN_COUNT 16
const int s_test_map[TEST_GPIO_PIN_COUNT] = {
    // GPIO_NUM_0,    //GPIO0   // Workaround: GPIO0 is strap pin, can not be used pullup/pulldown test.
    GPIO_NUM_2,    //GPIO2
    GPIO_NUM_4,    //GPIO4
    // GPIO_NUM_12,   //GPIO12  // Workaround: GPIO12 is strap pin, can not be used pullup/pulldown test.
    GPIO_NUM_13,   //GPIO13
    GPIO_NUM_14,   //GPIO14
    GPIO_NUM_15,   //GPIO15
    GPIO_NUM_25,   //GPIO25
    GPIO_NUM_26,   //GPIO26
    GPIO_NUM_27,   //GPIO27
    GPIO_NUM_32,   //GPIO32
    GPIO_NUM_33,   //GPIO33
    GPIO_NUM_34,   //GPIO34
    GPIO_NUM_35,   //GPIO35
    GPIO_NUM_36,   //GPIO36
    GPIO_NUM_37,   //GPIO37
    GPIO_NUM_38,   //GPIO38
    GPIO_NUM_39,   //GPIO39
};
#elif defined CONFIG_IDF_TARGET_ESP32S2
// Has no input-only rtcio pins, all pins support pull-up/down
#define RTCIO_SUPPORT_PU_PD(num)    1
#define TEST_GPIO_PIN_COUNT 20
const int s_test_map[TEST_GPIO_PIN_COUNT] = {
    // GPIO_NUM_0,    //GPIO0   // Workaround: GPIO0 is strap pin, can not be used pullup/pulldown test.
    GPIO_NUM_1,    //GPIO1
    GPIO_NUM_2,    //GPIO2
    GPIO_NUM_3,    //GPIO3
    GPIO_NUM_4,    //GPIO4
    GPIO_NUM_5,    //GPIO5
    GPIO_NUM_6,    //GPIO6
    GPIO_NUM_7,    //GPIO7
    GPIO_NUM_8,    //GPIO8
    GPIO_NUM_9,    //GPIO9
    GPIO_NUM_10,   //GPIO10
    GPIO_NUM_11,   //GPIO11
    GPIO_NUM_12,   //GPIO12
    GPIO_NUM_13,   //GPIO13
    GPIO_NUM_14,   //GPIO14
    GPIO_NUM_15,   //GPIO15
    GPIO_NUM_16,   //GPIO16
    GPIO_NUM_17,   //GPIO17
    // GPIO_NUM_18,   //GPIO18  // Workaround: IO18 is pullup outside in ESP32S2-Saola Runner.
    GPIO_NUM_19,   //GPIO19
    GPIO_NUM_20,   //GPIO20
    GPIO_NUM_21,   //GPIO21
};
#elif defined CONFIG_IDF_TARGET_ESP32S3
// Has no input-only rtcio pins, all pins support pull-up/down
#define RTCIO_SUPPORT_PU_PD(num)    1
#define TEST_GPIO_PIN_COUNT 21
const int s_test_map[TEST_GPIO_PIN_COUNT] = {
    // GPIO_NUM_0,    //GPIO0   // Workaround: GPIO0 is strap pin, can not be used pullup/pulldown test.
    GPIO_NUM_1,    //GPIO1
    GPIO_NUM_2,    //GPIO2
    GPIO_NUM_3,    //GPIO3
    GPIO_NUM_4,    //GPIO4
    GPIO_NUM_5,    //GPIO5
    GPIO_NUM_6,    //GPIO6
    GPIO_NUM_7,    //GPIO7
    GPIO_NUM_8,    //GPIO8
    GPIO_NUM_9,    //GPIO9
    GPIO_NUM_10,   //GPIO10
    GPIO_NUM_11,   //GPIO11
    GPIO_NUM_12,   //GPIO12
    GPIO_NUM_13,   //GPIO13
    GPIO_NUM_14,   //GPIO14
    GPIO_NUM_15,   //GPIO15
    GPIO_NUM_16,   //GPIO16
    GPIO_NUM_17,   //GPIO17
    GPIO_NUM_18,   //GPIO18
    GPIO_NUM_19,   //GPIO19
    GPIO_NUM_20,   //GPIO20
    GPIO_NUM_21,   //GPIO21
};
#elif CONFIG_IDF_TARGET_ESP32C6
// Has no input-only rtcio pins, all pins support pull-up/down
#define RTCIO_SUPPORT_PU_PD(num)    1
#define TEST_GPIO_PIN_COUNT 8
const int s_test_map[TEST_GPIO_PIN_COUNT] = {
    GPIO_NUM_0,    //GPIO0
    GPIO_NUM_1,    //GPIO1
    GPIO_NUM_2,    //GPIO2
    GPIO_NUM_3,    //GPIO3
    GPIO_NUM_4,    //GPIO4
    GPIO_NUM_5,    //GPIO5
    GPIO_NUM_6,    //GPIO6
    GPIO_NUM_7,    //GPIO7
};
#elif CONFIG_IDF_TARGET_ESP32H2
#define TEST_GPIO_PIN_COUNT 8
const int s_test_map[TEST_GPIO_PIN_COUNT] = {
    GPIO_NUM_7,    //GPIO7
    GPIO_NUM_8,    //GPIO8
    GPIO_NUM_9,    //GPIO9
    GPIO_NUM_10,   //GPIO10
    GPIO_NUM_11,   //GPIO11
    GPIO_NUM_12,   //GPIO12
    GPIO_NUM_13,   //GPIO13
    GPIO_NUM_14,   //GPIO14
};
#endif

#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
static const char *TAG = "rtcio_test";

#define RTCIO_CHECK(condition) TEST_ASSERT_MESSAGE((condition == ESP_OK), "ret is not ESP_OK")

#define TEST_COUNT 10

/*
 * Test output/input function.
 */
TEST_CASE("RTCIO_input/output_test", "[rtcio]")
{
    ESP_LOGI(TAG, "RTCIO input/output test");

    // init rtcio
    for (int i = 0; i < GPIO_PIN_COUNT; i++) {
        if (GPIO_IS_VALID_OUTPUT_GPIO(i) && rtc_gpio_is_valid_gpio(i)) {
            RTCIO_CHECK( rtc_gpio_init(i) );
            RTCIO_CHECK( rtc_gpio_set_direction(i, RTC_GPIO_MODE_INPUT_OUTPUT) );
            RTCIO_CHECK( rtc_gpio_pullup_dis(i) );
            RTCIO_CHECK( rtc_gpio_pulldown_dis(i) );
            ESP_LOGI(TAG, "gpio %d init", i);
        }
    }

    for (int cnt = 0; cnt < TEST_COUNT; cnt++) {
        int level = cnt % 2;
        ESP_LOGI(TAG, "RTCIO output level %d", level);
        for (int i = 0; i < GPIO_PIN_COUNT; i++) {
            if (GPIO_IS_VALID_OUTPUT_GPIO(i) && rtc_gpio_is_valid_gpio(i)) {
                RTCIO_CHECK( rtc_gpio_set_level(i, level) );
                vTaskDelay(10 / portTICK_PERIOD_MS);
                if (rtc_gpio_get_level(i) != level) {
                    ESP_LOGE(TAG, "RTCIO input/output test err, gpio%d", i);
                    TEST_FAIL();
                }
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    // Deinit rtcio
    for (int i = 0; i < GPIO_PIN_COUNT; i++) {
        if (GPIO_IS_VALID_OUTPUT_GPIO(i) && rtc_gpio_is_valid_gpio(i)) {
            RTCIO_CHECK( rtc_gpio_deinit(i) );
        }
    }
    ESP_LOGI(TAG, "RTCIO input/output test over");
}

/*
 * Test pullup/pulldown function.
 * Note: extern circuit should not connect.
 */
TEST_CASE("RTCIO_pullup/pulldown_test", "[rtcio]")
{
    ESP_LOGI(TAG, "RTCIO pullup/pulldown test");

    // init rtcio
    for (int i = 0; i < TEST_GPIO_PIN_COUNT; i++) {
        int num = rtc_io_number_get(s_test_map[i]);
        if (rtc_gpio_is_valid_gpio(s_test_map[i]) && num > 0 && RTCIO_SUPPORT_PU_PD(num)) {
            RTCIO_CHECK( rtc_gpio_init(s_test_map[i]) );
            RTCIO_CHECK( rtc_gpio_set_direction(s_test_map[i], RTC_GPIO_MODE_INPUT_ONLY) );
            RTCIO_CHECK( rtc_gpio_pullup_dis(s_test_map[i]) );
            RTCIO_CHECK( rtc_gpio_pulldown_dis(s_test_map[i]) );
            ESP_LOGI(TAG, "gpio %d init", s_test_map[i]);
        }
    }

    for (int cnt = 0; cnt < TEST_COUNT; cnt++) {
        int level = cnt % 2;
        ESP_LOGI(TAG, "RTCIO pull level %d", level);
        for (int i = 0; i < TEST_GPIO_PIN_COUNT; i++) {
            int num = rtc_io_number_get(s_test_map[i]);
            if (rtc_gpio_is_valid_gpio(s_test_map[i]) && num > 0 && RTCIO_SUPPORT_PU_PD(num)) {
                if (level) {
                    RTCIO_CHECK( rtc_gpio_pulldown_dis(s_test_map[i]) );
                    RTCIO_CHECK( rtc_gpio_pullup_en(s_test_map[i]) );
                } else {
                    RTCIO_CHECK( rtc_gpio_pullup_dis(s_test_map[i]) );
                    RTCIO_CHECK( rtc_gpio_pulldown_en(s_test_map[i]) );
                }
                vTaskDelay(20 / portTICK_PERIOD_MS);
                if (rtc_gpio_get_level(s_test_map[i]) != level) {
                    ESP_LOGE(TAG, "RTCIO pullup/pulldown test err, gpio%d", s_test_map[i]);
                    TEST_FAIL();
                }
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    // Deinit rtcio
    for (int i = 0; i < TEST_GPIO_PIN_COUNT; i++) {
        int num = rtc_io_number_get(s_test_map[i]);
        if (rtc_gpio_is_valid_gpio(s_test_map[i]) && num > 0 && RTCIO_SUPPORT_PU_PD(num)) {
            RTCIO_CHECK( rtc_gpio_deinit(s_test_map[i]) );
        }
    }
    ESP_LOGI(TAG, "RTCIO pullup/pulldown test over");
}

/*
 * Test output OD function.
 */
TEST_CASE("RTCIO_output_OD_test", "[rtcio]")
{
    ESP_LOGI(TAG, "RTCIO output OD test");

    // init rtcio
    for (int i = 0; i < GPIO_PIN_COUNT; i++) {
        if (GPIO_IS_VALID_OUTPUT_GPIO(i) && rtc_gpio_is_valid_gpio(i)) {
            RTCIO_CHECK( rtc_gpio_init(i) );
            RTCIO_CHECK( rtc_gpio_set_direction(i, RTC_GPIO_MODE_INPUT_OUTPUT_OD) );
            RTCIO_CHECK( rtc_gpio_pullup_en(i) );
            RTCIO_CHECK( rtc_gpio_pulldown_dis(i) );
            ESP_LOGI(TAG, "gpio %d init", i);
        }
    }

    for (int cnt = 0; cnt < TEST_COUNT; cnt++) {
        int level = cnt % 2;
        ESP_LOGI(TAG, "RTCIO output level %d", level);
        for (int i = 0; i < GPIO_PIN_COUNT; i++) {
            if (GPIO_IS_VALID_OUTPUT_GPIO(i) && rtc_gpio_is_valid_gpio(i)) {
                RTCIO_CHECK( rtc_gpio_set_level(i, level) );
                vTaskDelay(10 / portTICK_PERIOD_MS);
                if (rtc_gpio_get_level(i) != level) {
                    ESP_LOGE(TAG, "RTCIO output OD test err, gpio%d", i);
                    TEST_FAIL();
                }
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    // Deinit rtcio
    for (int i = 0; i < GPIO_PIN_COUNT; i++) {
        if (GPIO_IS_VALID_OUTPUT_GPIO(i) && rtc_gpio_is_valid_gpio(i)) {
            RTCIO_CHECK( rtc_gpio_deinit(i) );
        }
    }
    ESP_LOGI(TAG, "RTCIO output OD test over");
}

#if SOC_RTCIO_HOLD_SUPPORTED
/*
 * Test rtcio hold function.
 */
TEST_CASE("RTCIO_output_hold_test", "[rtcio]")
{
    ESP_LOGI(TAG, "RTCIO output hold test");

    // init rtcio
    for (int i = 0; i < GPIO_PIN_COUNT; i++) {
        if (GPIO_IS_VALID_OUTPUT_GPIO(i) && rtc_gpio_is_valid_gpio(i)) {
            RTCIO_CHECK( rtc_gpio_init(i) );
            RTCIO_CHECK( rtc_gpio_set_direction(i, RTC_GPIO_MODE_INPUT_OUTPUT_OD) );
            RTCIO_CHECK( rtc_gpio_pullup_en(i) );
            RTCIO_CHECK( rtc_gpio_pulldown_dis(i) );
            RTCIO_CHECK( rtc_gpio_set_level(i, 1) );
            ESP_LOGI(TAG, "gpio %d init, level 1", i);
        }
    }

    // hold all output rtcio.
    for (int i = 0; i < GPIO_PIN_COUNT; i++) {
        if (GPIO_IS_VALID_OUTPUT_GPIO(i) && rtc_gpio_is_valid_gpio(i)) {
            RTCIO_CHECK( rtc_gpio_hold_en(i) );
            vTaskDelay(10 / portTICK_PERIOD_MS);
            RTCIO_CHECK( rtc_gpio_set_level(i, 0) );
            ESP_LOGI(TAG, "RTCIO output pin hold, then set level 0");
            vTaskDelay(10 / portTICK_PERIOD_MS);
            if (rtc_gpio_get_level(i) == 0) {
                ESP_LOGE(TAG, "RTCIO hold test err, gpio%d", i);
                TEST_FAIL();
            }
        }
    }

    // unhold all rtcio.
    for (int i = 0; i < GPIO_PIN_COUNT; i++) {
        if (GPIO_IS_VALID_OUTPUT_GPIO(i) && rtc_gpio_is_valid_gpio(i)) {
            RTCIO_CHECK( rtc_gpio_hold_dis(i) );
        }
    }

    // check the unhold status
    for (int cnt = 0; cnt < 4; cnt++) {
        int level = cnt % 2;
        ESP_LOGI(TAG, "RTCIO output level %d", level);
        for (int i = 0; i < GPIO_PIN_COUNT; i++) {
            if (GPIO_IS_VALID_OUTPUT_GPIO(i) && rtc_gpio_is_valid_gpio(i)) {
                RTCIO_CHECK( rtc_gpio_set_level(i, level) );
                vTaskDelay(10 / portTICK_PERIOD_MS);
                if (rtc_gpio_get_level(i) != level) {
                    ESP_LOGE(TAG, "RTCIO output OD test err, gpio%d", i);
                    TEST_FAIL();
                }
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    // Deinit rtcio
    for (int i = 0; i < GPIO_PIN_COUNT; i++) {
        if (GPIO_IS_VALID_OUTPUT_GPIO(i) && rtc_gpio_is_valid_gpio(i)) {
            RTCIO_CHECK( rtc_gpio_deinit(i) );
        }
    }
    ESP_LOGI(TAG, "RTCIO hold test over");
}
#endif //SOC_RTCIO_HOLD_SUPPORTED
#endif //SOC_RTCIO_INPUT_OUTPUT_SUPPORTED

#if !CONFIG_IDF_TARGET_ESP32H2 // TODO: IDF-6268
// It is not necessary to test every rtcio pin, it will take too much ci testing time for deep sleep
// Only tests on s_test_map[TEST_RTCIO_DEEP_SLEEP_PIN_INDEX] pin
// (ESP32: IO25, ESP32S2, S3: IO6, C6: IO5, H2: IO12) these pads' default configuration is low level
#define TEST_RTCIO_DEEP_SLEEP_PIN_INDEX 5

static void rtcio_deep_sleep_hold_test_first_stage(void)
{
    printf("configure rtcio pin to hold during deep sleep");
    int io_num = s_test_map[TEST_RTCIO_DEEP_SLEEP_PIN_INDEX];

    TEST_ESP_OK(esp_sleep_enable_timer_wakeup(2000000));

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pin_bit_mask = (1ULL << io_num),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);

    gpio_set_level(io_num, 1);
    // Enable global persistence
    TEST_ESP_OK(gpio_hold_en(io_num));

    esp_deep_sleep_start();
}

static void rtcio_deep_sleep_hold_test_second_stage(void)
{
    int io_num = s_test_map[TEST_RTCIO_DEEP_SLEEP_PIN_INDEX];
    // Check reset reason is waking up from deepsleep
    TEST_ASSERT_EQUAL(ESP_RST_DEEPSLEEP, esp_reset_reason());
    // Pin should stay at high level after the deep sleep
    TEST_ASSERT_EQUAL_INT(1, gpio_get_level(io_num));

    gpio_hold_dis(io_num);
}

/*
 * Test rtcio hold function during deep sleep.
 * This test case can only check the hold state after waking up from deep sleep
 * If you want to check that the rtcio hold function works properly during deep sleep,
 * please use logic analyzer or oscilloscope
 */
TEST_CASE_MULTIPLE_STAGES("RTCIO_deep_sleep_output_hold_test", "[rtcio]",
                         rtcio_deep_sleep_hold_test_first_stage,
                         rtcio_deep_sleep_hold_test_second_stage)
#endif
