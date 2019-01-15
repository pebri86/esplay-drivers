#include "audio.h"

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "driver/i2s.h"
#include "driver/rtc_io.h"

static float Volume = 0.5f;
static volume_level volumeLevel = VOLUME_LEVEL1;
static int volumeLevels[] = {0, 125, 250, 500, 1000};

volume_level audio_volume_get()
{
    return volumeLevel;
}

void audio_volume_set(volume_level value)
{
    if (value >= VOLUME_LEVEL_COUNT)
    {
        printf("audio_volume_set: value out of range (%d)\n", value);
        abort();
    }

    volumeLevel = value;
    Volume = (float)volumeLevels[value] * 0.0001f;
}

void audio_volume_change()
{
    int level = (volumeLevel + 1) % VOLUME_LEVEL_COUNT;
    audio_volume_set(level);

    // settings_Volume_set(level);
}

void audio_init(int sample_rate)
{
    printf("%s: sample_rate=%d\n", __func__, sample_rate);

    // NOTE: buffer needs to be adjusted per AUDIO_SAMPLE_RATE
    i2s_config_t i2s_config = {
        //.mode = I2S_MODE_MASTER | I2S_MODE_TX,  // Only TX
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN,
        .sample_rate = sample_rate,
        .bits_per_sample = 16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // 2-channels
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = 6,
        //.dma_buf_len = 1472 / 2,  // (368samples * 2ch * 2(short)) = 1472
        .dma_buf_len = 512,  // (416samples * 2ch * 2(short)) = 1664
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, //Interrupt level 1
        .use_apll = false //1
    };

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);

    i2s_set_pin(I2S_NUM, NULL);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
}

void audio_submit(short* stereoAudioBuffer, int frameCount)
{

    short currentAudioSampleCount = frameCount*2;
   
    for (short i = 0; i < currentAudioSampleCount; ++i)
    {
        int sample = stereoAudioBuffer[i] * Volume;
        if (sample > 32767)
            sample = 32767;
        else if (sample < -32767)
            sample = -32767;
        
        sample  = sample ^ 0x8000u;

        stereoAudioBuffer[i] = (short)sample;
    }

    int len = currentAudioSampleCount * sizeof(int16_t);
    size_t count;

    i2s_write(I2S_NUM, (const char *)stereoAudioBuffer, len, &count, portMAX_DELAY);
    if ((int)count != len)
    {
        printf("i2s_write: count (%d) != len (%d)\n", (int)count, len);
        abort();
    }
}

void audio_terminate()
{
    i2s_zero_dma_buffer(I2S_NUM);
    i2s_stop(I2S_NUM);

    i2s_start(I2S_NUM);


    esp_err_t err = rtc_gpio_init(GPIO_NUM_25);
    err = rtc_gpio_init(GPIO_NUM_26);
    if (err != ESP_OK)
    {
        abort();
    }

    err = rtc_gpio_set_direction(GPIO_NUM_25, RTC_GPIO_MODE_OUTPUT_ONLY);
    err = rtc_gpio_set_direction(GPIO_NUM_26, RTC_GPIO_MODE_OUTPUT_ONLY);
    if (err != ESP_OK)
    {
        abort();
    }

    err = rtc_gpio_set_level(GPIO_NUM_25, 0);
    err = rtc_gpio_set_level(GPIO_NUM_26, 0);
    if (err != ESP_OK)
    {
        abort();
    }

}