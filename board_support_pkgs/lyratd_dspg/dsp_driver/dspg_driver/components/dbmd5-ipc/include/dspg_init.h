/*
 *      Copyright 2018, Espressif Systems (Shanghai) Pte Ltd.
 *  All rights regarding this code and its modifications reserved.
 *
 * This code contains confidential information of Espressif Systems
 * (Shanghai) Pte Ltd. No licenses or other rights express or implied,
 * by estoppel or otherwise are granted herein.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DSPG_INIT_H

#define DSPG_INIT_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef DBMD5_ESP_SR
#define AUDIO_BUF_SIZE 2048  //For ESP SR, need 2KB of chunk after Alexa is detected, 4KB write to RB is quite fast to check for Alexa for NN task
#else
#define AUDIO_BUF_SIZE 4096  //Default value
#endif

void dbmd5_init(QueueHandle_t queue);
void dbmd5_reset();
void dbmd5_enter_low_power();
void dbmd5_aec_init();
void dbmd5_mic_mute();
void dbmd5_mic_unmute();
void dbmd5_tap_to_talk();
void dbmd5_start_capture();
void dbmd5_stop_capture();
int dbmd5_stream_audio(uint8_t *buf, size_t max_len);
size_t dbmd5_get_ww_len();

#endif /* DSPG_INIT_H */
