#include <esp_log.h>
#include <string.h>
#include <voice_assistant_app_cb.h>
#include <resampling.h>
#include <audio_board.h>
#include "va_board.h"

#define I2S_PORT_NUM I2S_NUM_0
#define DES_BITS_PER_SAM 16
#define SRC_BITS_PER_SAM 16
/* The standard sampling rate we will setup for the hardware */
#define SAMPLING_RATE    48000
#define CONVERT_BUF_SIZE 1024
#define BUF_SZ (CONVERT_BUF_SIZE * 2)
static int16_t convert_buf[BUF_SZ];

static const char *TAG = "ZL38063_MH_CB";

/* XXX How does equaliser get managed? It must be optional, because it
 * pulls in an additional 3% of CPU overhead, and not all apps might
 * need it.
 */

int va_app_playback_data(va_resample_param_t *va_resample_param, void *buf, ssize_t len)
{
    memset(convert_buf, 0, BUF_SZ);
    static audio_resample_config_t resample = {0};
    int current_convert_block_len;
    int convert_block_len = 0;
    int send_offset = 0;
    size_t sent_len = 0;
    int conv_len = 0;

    if (va_resample_param->va_resample_ch == 1) {
        /* If mono recording, we need to up-sample, so need half the buffer empty, also uint16_t data*/
        convert_block_len = CONVERT_BUF_SIZE / 4;
    } else {
        /* If stereo, we won't need additional buffer space but data is uint16_t*/
        convert_block_len = CONVERT_BUF_SIZE / 2;
    }

    while (len) {
        current_convert_block_len = (convert_block_len > len) ? len : convert_block_len;
        if (current_convert_block_len & 1) {
            printf("Odd bytes in up sampling data, this should be backed up\n");
        }
        conv_len = audio_resample((short *)((char *)buf + send_offset), (short *)convert_buf, va_resample_param->va_resample_freq, SAMPLING_RATE,
                            current_convert_block_len / 2, BUF_SZ, va_resample_param->va_resample_ch, &resample);
        if (va_resample_param->va_resample_ch == 1) {
            conv_len = audio_resample_up_channel((short *)convert_buf, (short *)convert_buf, SAMPLING_RATE, SAMPLING_RATE, conv_len, BUF_SZ, &resample);
        }
        len -= current_convert_block_len;
        /* The reason send_offset and send_len are different is because we could be converting from 24K to 16K */
        send_offset += current_convert_block_len;
        if (DES_BITS_PER_SAM == SRC_BITS_PER_SAM) {
            i2s_write((i2s_port_t)I2S_PORT_NUM, (char *)convert_buf, conv_len * 2, &sent_len, portMAX_DELAY);
        } else {
            if (DES_BITS_PER_SAM > SRC_BITS_PER_SAM) {
                if (DES_BITS_PER_SAM % SRC_BITS_PER_SAM != 0) {
                    ESP_LOGE(TAG, "destination bits need to be multiple of source bits");
                    return -1;
                }
            } else {
                ESP_LOGE(TAG, "destination bits need to greater then and multiple of source bits");
                return -1;
            }
            i2s_write_expand((i2s_port_t)I2S_PORT_NUM, (char *)convert_buf, conv_len * 2, SRC_BITS_PER_SAM, DES_BITS_PER_SAM, &sent_len, portMAX_DELAY);
        }
    }
    return sent_len;
}