#include "power.h"

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_event.h"
#include "driver/rtc_io.h"

#include "gamepad.h"

static bool system_initialized = false;

void system_sleep()
{
    printf("%s: Entered.\n", __func__);

    // Wait for button release
    input_gamepad_state joystick;
    gamepad_read(&joystick);

    while (joystick.values[GAMEPAD_INPUT_MENU])
    {
        vTaskDelay(1);
        gamepad_read(&joystick);
    }

    // Configure button to wake
    printf("%s: Configuring deep sleep.\n", __func__);
#if 1
    esp_err_t err = esp_sleep_enable_ext0_wakeup(GPIO_NUM_2, 0);
#else
    const int ext_wakeup_pin_1 = ODROID_GAMEPAD_IO_MENU;
    const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin_1;

    esp_err_t err = esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask, ESP_EXT1_WAKEUP_ALL_LOW);
#endif
    if (err != ESP_OK)
    {
        printf("%s: esp_sleep_enable_ext0_wakeup failed.\n", __func__);
        abort();
    }

    err = rtc_gpio_pullup_en(GPIO_NUM_2);
    if (err != ESP_OK)
    {
        printf("%s: rtc_gpio_pullup_en failed.\n", __func__);
        abort();
    }


    // Isolate GPIO12 pin from external circuits. This is needed for modules
    // which have an external pull-up resistor on GPIO12 (such as ESP32-WROVER)
    // to minimize current consumption.
    rtc_gpio_isolate(GPIO_NUM_12);
#if 1
    //rtc_gpio_isolate(GPIO_NUM_34);
    //rtc_gpio_isolate(GPIO_NUM_35);
    //rtc_gpio_isolate(GPIO_NUM_0);
    //rtc_gpio_isolate(GPIO_NUM_39);
    //rtc_gpio_isolate(GPIO_NUM_14);
#endif

    // Sleep
    //esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    vTaskDelay(100);
    esp_deep_sleep_start();
}

void odroid_system_init()
{
    rtc_gpio_deinit(GPIO_NUM_2);
    //rtc_gpio_deinit(GPIO_NUM_14);

    system_initialized = true;
}