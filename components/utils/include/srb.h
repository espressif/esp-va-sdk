#pragma once

#include <ringbuf.h>

/* Special Ring Buffer: This is a ring-buffer that maintains offset
 * tracking of the ring buffer. We can register different points of
 * interest, called anchors within this ring buffer. These anchors are
 * can be fetched by the reader whenever it stumbles about it.
 */

struct srb_anchor {
    /* The offset at which this anchor is set. An anchor can be set at
     * any location which hasn't yet been read. It can be set at a
     * location which has already been written, but not read.
     */
    uint64_t offset;
    /* The data corresponding to this anchor */
    void *data;
};

struct srb_anchor_node {
    struct srb_anchor anchor;
    struct srb_anchor_node *next;
};

typedef struct {
    /* The ring buffer that holds the actual data */
    ringbuf_t *rb;
    /* The amount of data that has already been read from the above
     * ring buffer */
    uint64_t read_offset;
    /* Sorted list of anchors. For convenience of operations, the first node is empty */
    struct srb_anchor_node list;
    /* The lock that protects this data structure*/
    xSemaphoreHandle lock;
    xSemaphoreHandle read_lock;
} s_ringbuf_t;

/* Initialise the srb */
s_ringbuf_t *srb_init(const char *rb_name, uint32_t size);
/* Write to an srb */
int srb_write(s_ringbuf_t *srb, uint8_t *buf, int len, uint32_t ticks_to_wait);
/* Read from an srb. If you are at an anchor, an error
 * SRB_FETCH_ANCHOR will be returned. You are supposed to then use the
 * srb_get_anchor() to read the anchor first, before proceeding to
 * read further
 */
int srb_read(s_ringbuf_t *srb, uint8_t *buf, int len, uint32_t ticks_to_wait);
/* Put an anchor in the data stream at a particular offset */
int srb_put_anchor(s_ringbuf_t *srb, struct srb_anchor *anchor);
/* Read the current anchor */
int srb_get_anchor(s_ringbuf_t *srb, struct srb_anchor *anchor);
/* This puts the anchor at the current 'write' location. The value of
 * anchor->offset is ignored and overwritten with wherever the marker
 * was actually placed.
 */
int srb_put_anchor_at_current(s_ringbuf_t *srb, struct srb_anchor *anchor);
int srb_drain(s_ringbuf_t *srb, int drain_upto);
int srb_get_read_offset(s_ringbuf_t *srb);
int srb_get_filled(s_ringbuf_t *srb);
int srb_reset(s_ringbuf_t *srb);


/* What is 42? Is there any other way to expose this ? */
#define SRB_FETCH_ANCHOR    -42
#define SRB_NO_ANCHORS      -43
