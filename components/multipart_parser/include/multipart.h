/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
Header file for multipart parser
*/
#ifndef _MULTIPART_H_
#define _MULTIPART_H_

#include <stdlib.h>
/* enum for all the states of the parser */
enum multipart_state_machine {
    finding_data = 1,
    finding_boundary = 2,
    finding_header_name = 3,
    finding_header_value = 4,
    finding_first_CR = 5,
    finding_first_LF = 6,
    finding_second_CR = 7,
    finding_second_LF = 8,
    finding_next_char = 9,
    finding_colon = 10,
    finding_space_after_colon = 11,
    stream_almost_over = 12,
    stream_over = 13
};

/* Handle structure for multipart parser */
typedef struct multipart_handle_t {
    enum multipart_state_machine state;
    char boundary[100];
    char *boundary_init;
    char *current_data_start;
    int iterator;
    int matcher;
    int prev_matcher;
    int boundary_length;
    int current_data_size;
    int first_buffer;
    int first_data;
    int first_header_name;
    void *data;
} multipart_handle_t;


/* Callback structure for multipart parser */
typedef struct multipart_callbacks_t {
    void (*part_begin_cb) (multipart_handle_t *);
    void (*part_end_cb) (multipart_handle_t *);
    void (*header_name_cb) (multipart_handle_t *, const char *, size_t);
    void (*header_value_cb) (multipart_handle_t *, const char *, size_t);
    void (*data_cb) (multipart_handle_t *, const char *, size_t);
} multipart_callbacks_t;



/* Initialize function for the multipart parser. Initializing the handle. */
void multipart_init(multipart_handle_t *handle, char *boundary);

/* Function to parse the response. The buffer of the specified size (a part of the request) is passed to this function and the function does all the callbacks to the sections of the response. */
int multipart_parse_data(multipart_handle_t *handle, multipart_callbacks_t *cbs, char *buffer, int buffer_size);

#endif /* _MULTIPART_H_ */
