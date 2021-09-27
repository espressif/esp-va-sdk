// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _ESP_DOWNMIX_H_
#define _ESP_DOWNMIX_H_

#ifdef __cplusplus
extern "C"
{
#endif

/**
   Function Description
   Downmixing two audio files (defined as the base audio file and the newcome audio file) into one output audio file with the correspondingly gains.
   The newcome audio file will be downmixed into the base audio file.
   The number of channel(s) of the output audio file will be the same with that of the base audio file. The number of channel(s) of the newcome audio file will also be changed to the same with the base audio file, if it is different from that of the base audio file.
   The downmix process has 3 status:
        * Bypass Downmixing -- Only the base audio file will be played;
        * Switch on Downmixing -- The base audio file and the target audio file will first enter the transition period, during which the gains of these two files will be changed from the original level to the target level; then enter the stable period, sharing a same target gain;
        * Switch off Downmixing -- The base audio file and the target audio file will first enter the transition period, during which the gains of these two files will be changed back to their original levels; then enter the stable period, with their original gains, respectively. After that, the audio device enters the bybass status.
   Note that, the sample rates of the base audio file and the newcome audio file must be the same. Otherwise, an error occurs.
*/

typedef enum {
    DOWNMIX_INVALID    = -1,   /*!< Invalid status */
    DOWNMIX_BYPASS     = 0,    /*!< Only the base audio file will be played */
    DOWNMIX_SWITCH_ON  = 1,    /*!< The base audio file and the target audio file will first enter the transition period, during which the gains of these two files will be changed from the original level to the target level; then enter the stable period, sharing a same target gain */
    DOWNMIX_SWITCH_OFF = 2     /*!< The base audio file and the target audio file will first enter the transition period, during which the gains of these two files will be changed back to their original levels; then enter the stable period, with their original gains, respectively. After that, the audio device enters the bybass status */
} downmix_status_t;

/**
* @brief      Gain and transition period of the Downmix
*/
typedef struct {
    float set_dbgain[2];                /*!< The gain is expressed using the logarithmic decibel (dB) units (dB gain).
                                             When the downmixing is switched on, the gains of the audio files will be gradually changed from set_gain[0] to set_gain[1] in the transition period, and stay at set_gain[1] in the stable period;
                                             When the downmixing is switched off, the gains of the audio files will be gradually changed back from set_gain[1] to set_gain[0] in the transition period, and stay at set_gain[0] in the stable period;
                                               For the base audio file:
                                                  - set_gain[0]: the original gain of the base audio file before the downmixing process. Usually, if the downmixing is not used, set_gain[0] is usually set to 0 dB.
                                                  - set_gain[1]: the target gain of the base audio file after the downmixing process.
                                               For the newcome audio file:
                                                  - set_gain[0]: the original gain of the newcome audio file before the downmixing process. Usually, if the set_gain[0] is set to a relatively large value, such as -96 dB, it means the newcome audio file can be ignored.
                                                  - set_gain[1]: the target gain of the base audio file after the downmixing process. Usually, if the set_gain[0] is 0 dB, it means the newcome audio becomes the main audio source.
                                         */
    int transit_ms;                     /*!<the length of the transition period in milliseconds, which is the same for "switch on downloading" and "switch off downloading". */
} esp_downmix_gain_t;

/**
* @brief      Downmix information
*/
typedef struct {
    int sample_rate;                    /*!<Sample rate */
    int bits_num;                       /*!<Only 16-bit PCM audio is supported */
    int channels[2];                    /*!<channel[0]: the number of channel(s) of the base audio file; channel[1]: the number of channel(s) of the newcome audio file; The number of channel of the output audio file should be the same with channel[0]. */
    int dual_2_mono_select_ch;          /*!<When channels[0] and channel[1] are different, the number of channel(s) of the newcome audio file will change from channel[1] to channel[0], so it is the same with that of the base audio file. If mono channel -> dual channel, just copy. If dual channel -> mono channel, select one channel according to dual_2_mono_select_ch (only 0 or 1). */
    esp_downmix_gain_t downmix_gain[2]; /*!<Gain and transition period of the Downmix*/
} esp_downmix_info_t;

/**
* @brief      Creates the Downmix handle
*
* @param      downmix_info         the downmix information
*
* @return     The Downmix handle for esp_downmix_process and esp_downmix_close. NULL: creating Downmix handle failed
*/
void *esp_downmix_open(esp_downmix_info_t *downmix_info);

/**
* @brief      Processes the audio data through downmixing.
*
* @param      downmix_handle       the Downmix handle created and returned by esp_downmix_open()
* @param      inbuf0               the buffer that stores the base audio data, which is in PCM format
* @param      bytes_in_0           the length of base audio data (in bytes)
* @param      inbuf1               the buffer that stores the newcome audio data, which is in PCM format
* @param      bytes_in_1           the length of newcome audio data (in bytes).
* @param      outbuf               the buffer that stores the output audio data, which is in PCM format
* @param      downmix_status       the downmix status. For details, please check the description in downmix_status_t.
*
* @return     The length of the output audio data (in bytes), which is also in PCM format. A negative return value indicates an error has occurred.
*/
int esp_downmix_process(void *downmix_handle, unsigned char *inbuf0, int bytes_in_0,
                        unsigned char *inbuf1, int bytes_in_1, unsigned char *outbuf, downmix_status_t downmix_status);

/**
* @brief      Releases the Downmix handle.
*
* @param      downmix_handle       the Downmix handle.
*/
void esp_downmix_close(void *downmix_handle);

#ifdef __cplusplus
}
#endif

#endif
