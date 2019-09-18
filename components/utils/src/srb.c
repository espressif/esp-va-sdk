#include <stdio.h>
#include <string.h>
#include <esp_audio_mem.h>

#include "esp_log.h"
#include "esp_err.h"

#include <srb.h>

#define TAG "[srb]"
// #define DEBUG_ANCHORS 1

s_ringbuf_t *srb_init(const char *rb_name, uint32_t size)
{
    s_ringbuf_t *sr = esp_audio_mem_calloc(1, sizeof(*sr));
    if (!sr) {
        ESP_LOGE(TAG, "Failed to allocate SRB");
        return NULL;
    }

    memset(sr, 0, sizeof(*sr));

    sr->rb = rb_init(rb_name, size);
    if (!sr->rb) {
        ESP_LOGE(TAG, "Failed to allocate ring buffer");
        goto error;
    }

    sr->lock = xSemaphoreCreateMutex();
    if (!sr->lock) {
        ESP_LOGE(TAG, "Failed to create lock");
        goto error;
    }
    sr->read_lock = xSemaphoreCreateMutex();
    if (!sr->read_lock) {
        ESP_LOGE(TAG, "Failed to create read_lock");
        goto error;
    }

    return sr;

 error:
    if (sr) {
        if (sr->rb) {
            rb_cleanup(sr->rb);
        }
        esp_audio_mem_free(sr);
    }
    return NULL;
}

static struct srb_anchor_node *srb_node_new(struct srb_anchor *anchor)
{
    struct srb_anchor_node *n = esp_audio_mem_calloc(1, sizeof(*n));
    if (n) {
        n->anchor = *anchor;
        n->next = NULL;
    }
    return n;
}

static void srb_node_free(struct srb_anchor_node * n)
{
    esp_audio_mem_free(n);
}

static int srb_node_insert(s_ringbuf_t *srb, struct srb_anchor *anchor)
{
    struct srb_anchor_node *n = srb_node_new(anchor);
    if (!n) {
        ESP_LOGE(TAG, "Couldn't allocate node");
        return -1;
    }
    struct srb_anchor_node *current = &srb->list;
    while (current) {
        /* This will ensure that if we have 2 anchors at the same
         * offset, the one that came later is added later
         */
        if ((current->next == NULL) || current->next->anchor.offset > n->anchor.offset) {
            struct srb_anchor_node *next = current->next;
            current->next = n;
            n->next = next;
            break;
        }
        current = current->next;
    }
    return 0;
}

int srb_read(s_ringbuf_t *srb, uint8_t *buf, int len, uint32_t ticks_to_wait)
{
    xSemaphoreTake(srb->read_lock, portMAX_DELAY);
    xSemaphoreTake(srb->lock, portMAX_DELAY);
    if (srb->list.next) {
        /* If an anchor exists */
        int64_t anchor_distance = srb->list.next->anchor.offset - srb->read_offset;
        if (anchor_distance <= 0) {
            /* We are at the anchor, this needs to be fetched first */
            xSemaphoreGive(srb->lock);
            xSemaphoreGive(srb->read_lock);
            return SRB_FETCH_ANCHOR;
        }
        if (len > anchor_distance) {
            /* Need to perform partial read */
            len = anchor_distance;
        }
    }
    xSemaphoreGive(srb->lock);

    /* It is possible that when we give the lock, someone else does srb_put_anchor() at an offset whicgh is less than (len + srb->read_offset). So, ideally srb_read() should return SRB_FETCH_ANCHOR. But we treat this as if the read will happen first and then in the next srb_read, we will return SRB_FETCH_ANCHOR as the anchor_distance would be negative. */
    int ret = rb_read(srb->rb, buf, len, ticks_to_wait);

    xSemaphoreTake(srb->lock, portMAX_DELAY);
    if (ret > 0) {
        srb->read_offset += ret;
    }
    // printf("srb_read returned: %d, read_offset: %lld\n", ret, srb->read_offset);
    xSemaphoreGive(srb->lock);
    xSemaphoreGive(srb->read_lock);
    return ret;
}

int srb_write(s_ringbuf_t *srb, uint8_t *buf, int len, uint32_t ticks_to_wait)
{
    return rb_write(srb->rb, buf, len, ticks_to_wait);
}

int srb_get_anchor(s_ringbuf_t *srb, struct srb_anchor *anchor)
{
    int rc = 0;
    xSemaphoreTake(srb->lock, portMAX_DELAY);
#ifdef DEBUG_ANCHORS
    printf("%s: Debug list:\n", TAG);
    struct srb_anchor_node *current = srb->list.next;
    while (current) {
        printf("   [%lld %p]\n", current->anchor.offset, current->anchor.data);
        current = current->next;
    }
#endif
    if (! srb->list.next) {
        rc = SRB_NO_ANCHORS;
        goto err_return;
    }

    int64_t anchor_distance = srb->list.next->anchor.offset - srb->read_offset;
    if (anchor_distance > 0) {
        ESP_LOGE(TAG, "No anchor at this point");
        rc =SRB_NO_ANCHORS;
        goto err_return;
    }

    struct srb_anchor_node *n = srb->list.next;
    *anchor = n->anchor;
    srb->list.next = n->next;
    srb_node_free(n);

 err_return:
    xSemaphoreGive(srb->lock);
    return rc;
}

/* Assumes lock is taken outside */
int __srb_put_anchor(s_ringbuf_t *srb, struct srb_anchor *anchor)
{
    int rc = 0;
    if (srb->read_offset >= anchor->offset) {
        /* If a reader was sleeping on rb_read() at this point, ideally the put_anchor() should wake that reader as well. */
        ESP_LOGI(TAG, "Setting anchor at current or at a point that is already read.");
        rb_wakeup_reader(srb->rb);
    }
    rc = srb_node_insert(srb, anchor);
    return rc;
}

int srb_put_anchor(s_ringbuf_t *srb, struct srb_anchor *anchor)
{
    int rc = 0;
    xSemaphoreTake(srb->lock, portMAX_DELAY);
    rc = __srb_put_anchor(srb, anchor);
    xSemaphoreGive(srb->lock);
    return rc;
}

int srb_put_anchor_at_current(s_ringbuf_t *srb, struct srb_anchor *anchor)
{
    int rc = 0;
    xSemaphoreTake(srb->lock, portMAX_DELAY);
    /* Calculate the current write offset */
    anchor->offset = srb->read_offset + rb_filled(srb->rb);
    rc = __srb_put_anchor(srb, anchor);
    xSemaphoreGive(srb->lock);
    return rc;
}

/* This will drain the data but leave the anchors as it is. So, on the next read, all the anchors whose offsets have already been passed will be returned one by one. */
int srb_drain(s_ringbuf_t *srb, int drain_upto)
{
    int ret = 0, len = 0;
    xSemaphoreTake(srb->read_lock, portMAX_DELAY);
    xSemaphoreTake(srb->lock, portMAX_DELAY);
    if (srb->read_offset < drain_upto) {
        do {
            len = drain_upto - srb->read_offset;
            ret = rb_read(srb->rb, NULL, len, 0);           // Passing buf as NULL will drain the bytes.
            if (ret > 0) {
                srb->read_offset += ret;
                ESP_LOGI(TAG, "Draining data");
            }
            /* We are looping since the buffer might be full and the codec might have written half the data. So, when we read from the buffer, the codec writes the remaining data, so we need to read and flush again. */
            /* We are getting -3 which is wakeup reader on the first read */
        } while ((ret > 0 || ret == -3) && srb->read_offset < drain_upto);
    }
    ret = srb->read_offset;
    xSemaphoreGive(srb->lock);
    xSemaphoreGive(srb->read_lock);
    return ret;
}

int srb_get_read_offset(s_ringbuf_t *srb)
{
    int ret = 0;
    xSemaphoreTake(srb->lock, portMAX_DELAY);
    ret = srb->read_offset;
    xSemaphoreGive(srb->lock);
    return ret;
}

int srb_get_filled(s_ringbuf_t *srb)
{
    int ret = 0;
    xSemaphoreTake(srb->lock, portMAX_DELAY);
    ret = rb_filled(srb->rb);
    xSemaphoreGive(srb->lock);
    return ret;
}

/* This will reset the rb but leave the anchors as it is. So, when the offset at which the anchors are present is reached again, that anchor will be returned and it will be required to handle it at that time. We should add a parameter which will specify if we also want to clear the offsets. */
int srb_reset(s_ringbuf_t *srb)
{
    /* TODO: There will be a corner case here where srb_read has just returned from rb_read and we abort. So, srb_read will try to take the lock but will not be able to as we have already taken it here. Then we reset and give the lock. And after that srb_read will take the lock and update read_offset. This is wrong and needs to be handled. We need to separate abort and reset. and after doing abort, wait for everything to stop and then do reset. */
    xSemaphoreTake(srb->lock, portMAX_DELAY);
    rb_abort(srb->rb);
    srb->read_offset = 0;
    rb_reset(srb->rb);
    xSemaphoreGive(srb->lock);
    return 0;
}
