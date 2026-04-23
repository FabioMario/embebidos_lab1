// Copyright 2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "led_strip.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"

static const char *TAG = "ws2812";
#define STRIP_CHECK(a, str, goto_tag, ret_value, ...)                             \
    do                                                                            \
    {                                                                             \
        if (!(a))                                                                 \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            ret = ret_value;                                                      \
            goto goto_tag;                                                        \
        }                                                                         \
    } while (0)

#define RMT_LED_STRIP_RESOLUTION_HZ (10000000) // 10MHz -> 1 tick = 0.1us
#define RMT_MEM_BLOCK_SYMBOLS (64)
#define RMT_TRANS_QUEUE_DEPTH (4)

#define KALUGA_RGB_LED_PIN GPIO_NUM_45
#define KALUGA_RGB_LED_NUMBER 1

static const rmt_symbol_word_t ws2812_zero = {
    .level0 = 1,
    .duration0 = 0.3 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000,
    .level1 = 0,
    .duration1 = 0.9 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000,
};

static const rmt_symbol_word_t ws2812_one = {
    .level0 = 1,
    .duration0 = 0.9 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000,
    .level1 = 0,
    .duration1 = 0.3 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000,
};

static const rmt_symbol_word_t ws2812_reset = {
    .level0 = 0,
    .duration0 = RMT_LED_STRIP_RESOLUTION_HZ / 1000000 * 50 / 2,
    .level1 = 0,
    .duration1 = RMT_LED_STRIP_RESOLUTION_HZ / 1000000 * 50 / 2,
};

typedef struct
{
    led_strip_t parent;
    rmt_channel_handle_t rmt_channel;
    rmt_encoder_handle_t encoder;
    uint32_t strip_len;
    uint8_t buffer[0];
} ws2812_t;

static size_t ws2812_encode_callback(const void *data, size_t data_size,
                                     size_t symbols_written, size_t symbols_free,
                                     rmt_symbol_word_t *symbols, bool *done, void *arg)
{
    (void)arg;

    // Need 8 symbols to encode one byte.
    if (symbols_free < 8)
    {
        return 0;
    }

    size_t data_pos = symbols_written / 8;
    const uint8_t *data_bytes = (const uint8_t *)data;

    if (data_pos < data_size)
    {
        size_t symbol_pos = 0;
        for (int bitmask = 0x80; bitmask != 0; bitmask >>= 1)
        {
            symbols[symbol_pos++] = (data_bytes[data_pos] & bitmask) ? ws2812_one : ws2812_zero;
        }
        return symbol_pos;
    }

    symbols[0] = ws2812_reset;
    *done = true;
    return 1;
}

esp_err_t ws2812_set_pixel(led_strip_t *strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue)
{
    esp_err_t ret = ESP_OK;
    ws2812_t *ws2812 = (ws2812_t *)strip;
    STRIP_CHECK(index < ws2812->strip_len, "index out of the maximum number of leds", err, ESP_ERR_INVALID_ARG);
    uint32_t start = index * 3;
    // WS2812 expects color bytes in GRB order.
    ws2812->buffer[start + 0] = green & 0xFF;
    ws2812->buffer[start + 1] = red & 0xFF;
    ws2812->buffer[start + 2] = blue & 0xFF;

    return ESP_OK;
err:
    return ret;
}

esp_err_t ws2812_refresh(led_strip_t *strip, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ws2812_t *ws2812 = (ws2812_t *)strip;
    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };

    STRIP_CHECK(rmt_transmit(ws2812->rmt_channel, ws2812->encoder, ws2812->buffer, ws2812->strip_len * 3, &tx_config) == ESP_OK,
                "transmit RMT samples failed", err, ESP_FAIL);

    return rmt_tx_wait_all_done(ws2812->rmt_channel, (int)timeout_ms);
err:
    return ret;
}

esp_err_t ws2812_clear(led_strip_t *strip, uint32_t timeout_ms)
{
    ws2812_t *ws2812 = (ws2812_t *)strip;
    // Write zero to turn off all leds
    memset(ws2812->buffer, 0, ws2812->strip_len * 3);
    return ws2812_refresh(strip, timeout_ms);
}

esp_err_t ws2812_del(led_strip_t *strip)
{
    ws2812_t *ws2812 = (ws2812_t *)strip;

    if (ws2812->encoder)
    {
        rmt_del_encoder(ws2812->encoder);
    }
    if (ws2812->rmt_channel)
    {
        rmt_disable(ws2812->rmt_channel);
        rmt_del_channel(ws2812->rmt_channel);
    }
    free(ws2812);
    return ESP_OK;
}

led_strip_t *led_strip_new_rmt_ws2812(const led_strip_config_t *config)
{
    led_strip_t *ret = NULL;
    ws2812_t *ws2812 = NULL;
    STRIP_CHECK(config, "configuration can't be null", err, NULL);

    // 24 bits per led
    uint32_t ws2812_size = sizeof(ws2812_t) + config->max_leds * 3;
    ws2812 = calloc(1, ws2812_size);
    STRIP_CHECK(ws2812, "request memory for ws2812 failed", err, NULL);

    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = (gpio_num_t)(uintptr_t)config->dev,
        .mem_block_symbols = RMT_MEM_BLOCK_SYMBOLS,
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = RMT_TRANS_QUEUE_DEPTH,
    };
    STRIP_CHECK(rmt_new_tx_channel(&tx_chan_config, &ws2812->rmt_channel) == ESP_OK, "create RMT TX channel failed", err, NULL);

    const rmt_simple_encoder_config_t simple_encoder_cfg = {
        .callback = ws2812_encode_callback,
    };
    STRIP_CHECK(rmt_new_simple_encoder(&simple_encoder_cfg, &ws2812->encoder) == ESP_OK, "create RMT encoder failed", err, NULL);
    STRIP_CHECK(rmt_enable(ws2812->rmt_channel) == ESP_OK, "enable RMT TX channel failed", err, NULL);

    ws2812->strip_len = config->max_leds;

    ws2812->parent.set_pixel = ws2812_set_pixel;
    ws2812->parent.refresh = ws2812_refresh;
    ws2812->parent.clear = ws2812_clear;
    ws2812->parent.del = ws2812_del;

    return &ws2812->parent;
err:
    if (ws2812)
    {
        if (ws2812->encoder)
        {
            rmt_del_encoder(ws2812->encoder);
        }
        if (ws2812->rmt_channel)
        {
            rmt_disable(ws2812->rmt_channel);
            rmt_del_channel(ws2812->rmt_channel);
        }
        free(ws2812);
    }
    return ret;
}

esp_err_t led_rgb_init(led_strip_t **strip)
{
    ESP_LOGI(TAG, "Initializing embedded WS2812");
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(KALUGA_RGB_LED_NUMBER, (led_strip_dev_t)KALUGA_RGB_LED_PIN);
    *strip = led_strip_new_rmt_ws2812(&strip_config);

    if (!(*strip))
    {
        ESP_LOGE(TAG, "install WS2812 driver failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "install WS2812 driver correct");

    return (*strip)->clear((*strip), 100);
}