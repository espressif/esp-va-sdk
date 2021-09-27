#include <string.h>
#include <esp_system.h>

#include <esp_audio_mem.h>
#include <UUIDGeneration.h>

#define UUID_VERSION_VALUE  4<<4
#define UUID_VARIANT_VALUE  2<<6

char *generateUUID() {
    /* Format for UUID is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx */
    /* With 7th-bit of 3rd octet set to signify truely random version of UUID and
     * 8th-bit of 4th octet set to signify variant value */
    uint8_t uuid[16] = {0};
    uint32_t random_no = esp_random();
    uint16_t *tmp_ptr = (uint16_t *)&random_no;
    memcpy(&uuid[0], &random_no, sizeof(uint32_t));

    random_no = esp_random();
    memcpy(&uuid[4], &random_no, sizeof(uint16_t));
    uint16_t tmp_num;
    memcpy(&tmp_num, tmp_ptr + 1, sizeof(uint16_t));
    tmp_num |= UUID_VERSION_VALUE;
    memcpy(&uuid[6], &tmp_num, sizeof(uint16_t));

    random_no = esp_random();
    memcpy(&tmp_num, &random_no, sizeof(uint16_t));
    tmp_num |= UUID_VARIANT_VALUE;
    memcpy(&uuid[8], &tmp_num, sizeof(uint16_t));

    memcpy(&uuid[10], tmp_ptr + 1, sizeof(uint16_t));
    random_no = esp_random();
    memcpy(&uuid[12], &random_no, sizeof(uint32_t));

    char *uuid_str = esp_audio_mem_calloc(1, UUID_MAX_LEN);
    snprintf(uuid_str, UUID_MAX_LEN, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7],
            uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);

    return uuid_str;
}
