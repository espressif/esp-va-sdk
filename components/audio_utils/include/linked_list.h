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

#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

typedef struct list_node_t {
    struct list_node_t *next;
    struct list_node_t *prev;
} list_node_t;

/*
 * @brief: append a node to the end of the list
 *
 */
void linked_list_add_last(list_node_t *root, list_node_t *newTree);

/*
 * @brief: get the last node in the list
 * @note: might get the root
 *
 */
list_node_t *linked_list_get_last(list_node_t *root);

/*
 * @brief: get the first node in the list
 * @note: exclude root
 *
 */
list_node_t *linked_list_get_first(list_node_t *root);

/*
 * @brief: remove a node from the list
 * @note: exclude root (should not remove root)
 *
 */
void linked_list_remove_node(list_node_t *root, list_node_t *tree);

void linked_list_clear(list_node_t *root);

/*
 * @brief: get the number of nodes in the list (used for debug purposes)
 * @note: exclude root
 */
int linked_list_get_size(list_node_t *root);

/*
 * @brief: move a node in the list to the end of list
 * @note: do not move root
 */
void linked_list_move_to_last(list_node_t *node);
int linked_list_each(list_node_t **root_ptr);
#endif
