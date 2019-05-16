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
This file has the function definations required for the multipart parser
*/

#include <stdio.h>
#include <multipart.h>

static const char *TAG = "[multipart]";

/* Initialize the handle */
void multipart_init(multipart_handle_t *handle, char *boundary)
{
    handle->state = finding_data;
    handle->iterator = 0;
    handle->matcher = 2;
    handle->prev_matcher = 0;
    handle->boundary_length = 0;
    handle->boundary_init = "\r\n--";                                                  //the actual boundary is ("\r\n" + "--" + boundary)
    while (handle->boundary_init[handle->iterator] != '\0') {
        handle->boundary[handle->iterator] = handle->boundary_init[handle->iterator];
        handle->iterator++;
    }
    handle->boundary_length += handle->iterator;
    handle->iterator = 0;
    while (boundary[handle->iterator] != '\0') {
        handle->boundary[handle->iterator + handle->boundary_length] = boundary[handle->iterator];
        handle->iterator++;
    }
    handle->boundary_length += handle->iterator;
    handle->boundary[handle->boundary_length] = '\0';
    handle->iterator = 0;
    handle->current_data_start = NULL;
    handle->current_data_size = 0;
    handle->first_buffer = 1;
    handle->first_data = 1;
    handle->first_header_name = 1;
}

/* Function to parse the response. The buffer of the specified size (a part of the request) is passed to this function and the function does all the callbacks to the sections of the response */
int multipart_parse_data(multipart_handle_t *handle, multipart_callbacks_t *cbs, char *buffer, int buffer_size)
{
    handle->iterator = 0;
    handle->current_data_start = buffer;
    handle->current_data_size = 1;

    while (handle->iterator < buffer_size && handle->state != stream_over) {
        switch (handle->state) {

        case finding_data :
            if (buffer[handle->iterator] == handle->boundary[handle->matcher]) {
                handle->matcher++;
                handle->state = finding_boundary;
            }
            if (handle->first_data) {
                handle->current_data_start = (buffer + handle->iterator);
                handle->current_data_size = 1;
                handle->first_data = 0;
            }
            break;

        case finding_boundary :
            if (buffer[handle->iterator] == handle->boundary[handle->matcher]) {
                handle->matcher++;
                if (handle->matcher == handle->boundary_length) {
                    if (!handle->first_buffer) {
                        if (handle->current_data_size - (handle->matcher - handle->prev_matcher) > 0) {
                            cbs->data_cb(handle, handle->current_data_start, handle->current_data_size - (handle->matcher - handle->prev_matcher));
                        }
                        cbs->data_cb(handle, NULL, 0);
                        cbs->part_end_cb(handle);
                    } else {
                        handle->first_buffer = 0;
                    }
                    handle->state = finding_first_CR;
                    handle->matcher = 0;
                }
            } else {
                if (handle->prev_matcher > 0) {
                    cbs->data_cb(handle, handle->boundary, handle->prev_matcher);
                    handle->prev_matcher = 0;
                }
                handle->state = finding_data;
                handle->matcher = 0;
            }
            break;

        case finding_header_name :
            if (buffer[handle->iterator] == ':') {
                handle->current_data_size--;
                if (handle->current_data_size > 0) {
                    cbs->header_name_cb(handle, handle->current_data_start, handle->current_data_size);
                }
                cbs->header_name_cb(handle, NULL, 0);
                handle->state = finding_colon;
                handle->iterator--;
            }
            break;

        case finding_header_value :
            if (buffer[handle->iterator] == '\r') {
                handle->current_data_size--;
                if (handle->current_data_size > 0) {
                    cbs->header_value_cb(handle, handle->current_data_start, handle->current_data_size);
                }
                cbs->header_value_cb(handle, NULL, 0);
                handle->state = finding_first_CR;
                handle->iterator--;
            }
            break;

        case finding_first_CR :
            if (buffer[handle->iterator] == '\r') {
                if (handle->first_header_name) {
                    cbs->part_begin_cb(handle);
                    handle->first_header_name = 0;
                    handle->prev_matcher = 0;
                    handle->matcher = 0;
                }
                handle->state = finding_first_LF;
            } else if (buffer[handle->iterator] == '-') {
                handle->state = stream_almost_over;
            } else {
                printf("%s: Found boundary in data. Incorrect data.\n", TAG);
            }
            break;

        case finding_first_LF :
            if (buffer[handle->iterator] == '\n') {
                handle->state = finding_next_char;
            } else {
                printf("%s: No LF after CR.\n", TAG);
            }
            break;

        case finding_next_char :
            if (buffer[handle->iterator] == '\r') {
                handle->state = finding_second_CR;
            } else {
                handle->state = finding_header_name;
                handle->current_data_start = (buffer + handle->iterator);
                handle->current_data_size = 0;
            }
            handle->iterator--;
            break;

        case finding_colon :
            if (buffer[handle->iterator] != ':') {
                handle->state = finding_space_after_colon;
                handle->iterator--;
            }
            break;

        case finding_space_after_colon :
            if (buffer[handle->iterator] != ' ') {
                handle->state = finding_header_value;
                handle->current_data_start = (buffer + handle->iterator);
                handle->current_data_size = 1;
            }
            break;

        case finding_second_CR :
            if (buffer[handle->iterator] == '\r') {
                handle->state = finding_second_LF;
            }
            break;

        case finding_second_LF :
            if (buffer[handle->iterator] == '\n') {
                handle->state = finding_data;
                handle->first_data = 1;
                handle->first_header_name = 1;
            }
            break;

        case stream_almost_over :
            if (buffer[handle->iterator] == '-') {
                handle->state = stream_over;
            }
            break;

        case stream_over :
            break;

        default :
            break;

        }

        handle->iterator++;
        handle->current_data_size++;
    }


    if (handle->iterator == buffer_size) {
        handle->current_data_size--;
        switch (handle->state) {

        case finding_data :
            if (handle->current_data_size > 0 && handle->first_data == 0) {
                cbs->data_cb(handle, handle->current_data_start, handle->current_data_size);
            }
            break;

        case finding_boundary :
            if (handle->current_data_size - (handle->matcher - handle->prev_matcher) > 0) {
                cbs->data_cb(handle, handle->current_data_start, handle->current_data_size - (handle->matcher - handle->prev_matcher));
            }
            handle->prev_matcher = handle->matcher;
            break;

        case finding_header_name :
            if (handle->current_data_size > 0) {
                cbs->header_name_cb(handle, handle->current_data_start, handle->current_data_size);
            }
            break;

        case finding_header_value :
            if (handle->current_data_size > 0) {
                cbs->header_value_cb(handle, handle->current_data_start, handle->current_data_size);
            }
            break;

        default :
            break;
        }
    }

    return buffer_size;
}
