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

#ifndef _APP_DSP_H_
#define _APP_DSP_H_

#include <alexa_app_cb.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum dsp_called_states {
    ALEXA_CAN_START = (ALEXA_END_STATES + 1),
} dsp_called_states_t;

void app_dsp_init(void);

void app_dsp_send_recognize();

void app_dsp_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* _APP_DSP_H_ */
