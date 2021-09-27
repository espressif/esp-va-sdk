// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _SMART_HOME_H_
#define _SMART_HOME_H_

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

#define SMART_HOME_PARAM_NAME           "esp.param.name"

#define SMART_HOME_PARAM_FLAG_VALUE_CHANGE   0x01

/* Smart Home handles */
typedef size_t smart_home_handle_t;
typedef smart_home_handle_t smart_home_node_t;
typedef smart_home_handle_t smart_home_device_t;
typedef smart_home_handle_t smart_home_param_t;
typedef smart_home_handle_t smart_home_attr_t;

typedef struct {
    /** Name of the Node */
    char *name;
    /** Type of the Node */
    char *type;
    /** Firmware Version (Optional). If not set, PROJECT_VER is used as default (recommended)*/
    char *fw_version;
    /** Model (Optional). If not set, PROJECT_NAME is used as default (recommended)*/
    char *model;
} smart_home_node_info_t;

/** Smart Home Parameter Value type */
typedef enum {
    /** Invalid */
    SMART_HOME_VAL_TYPE_INVALID = 0,
    /** Boolean */
    SMART_HOME_VAL_TYPE_BOOLEAN,
    /** Integer. Mapped to a 32 bit signed integer */
    SMART_HOME_VAL_TYPE_INTEGER,
    /** Floating point number */
    SMART_HOME_VAL_TYPE_FLOAT,
    /** NULL terminated string */
    SMART_HOME_VAL_TYPE_STRING,
    /** NULL terminated JSON Object string Eg. {"name":"value"} */
    SMART_HOME_VAL_TYPE_OBJECT,
    /** NULL terminated JSON Array string Eg. [1,2,3] */
    SMART_HOME_VAL_TYPE_ARRAY,
} smart_home_val_type_t;

/* Smart Home Value */
typedef union {
    /** Boolean */
    bool b;
    /** Integer */
    int i;
    /** Float */
    float f;
    /** NULL terminated string */
    char *s;
} smart_home_val_t;

/* Smart Home Parameter Value */
typedef struct {
    /** Type of Value */
    smart_home_val_type_t type;
    /** Actual value. Depends on the type */
    smart_home_val_t val;
} smart_home_param_val_t;

/** Param property flags */
typedef enum {
    /* Param is writable */
    SMART_HOME_PROP_FLAG_WRITE = (1 << 0),
    /* Param is readable */
    SMART_HOME_PROP_FLAG_READ = (1 << 1),
    /* Param maintains history as time-series data (This will be used in future) */
    SMART_HOME_PROP_FLAG_TIME_SERIES = (1 << 2),
    /* Param maintains state after reboot */
    SMART_HOME_PROP_FLAG_PERSIST = (1 << 3)
} smart_home_param_property_flags_t;

/** Parameter read/write request source */
typedef enum {
    /** Request triggered in the init sequence i.e. when a value is found
     * in persistent memory for parameters with PROP_FLAG_PERSIST.
     */
    SMART_HOME_REQ_SRC_INIT,
    /** Request received from cloud */
    SMART_HOME_REQ_SRC_CLOUD,
} smart_home_req_src_t;

/** Write request Context */
typedef struct {
    /** Source of request */
    smart_home_req_src_t src;
} smart_home_write_ctx_t;

/** Read request context */
typedef struct {
    /** Source of request */
    smart_home_req_src_t src;
} smart_home_read_ctx_t;

/** Callback for parameter value write requests.
 *
 * The callback should call the smart_home_param_update() API if the new value is to be set.
 *
 * @param[in] device Device handle.
 * @param[in] param Parameter handle.
 * @param[in] param Pointer to \ref smart_home_param_val_t. Use appropriate elements as per the value type.
 * @param[in] priv_data Pointer to the private data paassed while creating the device.
 * @param[in] ctx Context associated with the request.
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
typedef esp_err_t (*smart_home_device_write_cb_t)(const smart_home_device_t *device, const smart_home_param_t *param, const smart_home_param_val_t val, void *priv_data, smart_home_write_ctx_t *ctx);

/** Callback for parameter value changes
 *
 * The callback should call the smart_home_param_update() API if the new value is to be set.
 *
 * @note Currently, the read callback never gets invoked. This callback will be used in future.
 *
 * @param[in] device Device handle.
 * @param[in] param Parameter handle.
 * @param[in] priv_data Pointer to the private data passed while creating the device.
 * @param[in] ctx Context associated with the request.
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
typedef esp_err_t (*smart_home_device_read_cb_t)(const smart_home_device_t *device, const smart_home_param_t *param, void *priv_data, smart_home_read_ctx_t *ctx);

/**
 * Initialise a Boolean value
 *
 * @param[in] bval Initialising value.
 *
 * @return Value structure.
 */
smart_home_param_val_t smart_home_bool(bool bval);

/**
 * Initialise an Integer value
 *
 * @param[in] ival Initialising value.
 *
 * @return Value structure.
 */
smart_home_param_val_t smart_home_int(int ival);

/**
 * Initialise a Float value
 *
 * @param[in] fval Initialising value.
 *
 * @return Value structure.
 */
smart_home_param_val_t smart_home_float(float fval);

/**
 * Initialise a String value
 *
 * @param[in] sval Initialising value.
 *
 * @return Value structure.
 */
smart_home_param_val_t smart_home_str(const char *sval);

/**
 * Initialise a json object value
 *
 * @note the object will not be validated internally. it is the application's
 * responsibility to ensure that the object is a valid json object.
 * eg. smart_home_obj("{\"name\":\"value\"}");
 *
 * param[in] val initialising value
 *
 * @return value structure
 */
smart_home_param_val_t smart_home_obj(const char *val);

/**
 * Initialise a json array value
 *
 * @note the array will not be validated internally. it is the application's
 * responsibility to ensure that the array is a valid json array.
 * eg. smart_home_array("[1,2,3]");
 *
 * param[in] val initialising value
 *
 * @return value structure
 */
smart_home_param_val_t smart_home_array(const char *val);

/** Get a handle to the Node
 *
 * This API returns handle to a node created using smart_home_node_init().
 *
 * @return Node handle on success.
 * @return NULL in case of failure.
 */
const smart_home_node_t *smart_home_get_node(void);

/** Add Node attribute
 *
 * Adds a new attribute as the metadata for the node. For the sake of simplicity,
 * only string values are allowed.
 *
 * @param node Node handle.
 * @param[in] attr_name Name of the attribute.
 * @param[in] val Value for the attribute.
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t smart_home_node_add_attribute(const smart_home_node_t *node, const char *attr_name, const char *val);

/**
 * Create a Device
 *
 * This API will create a virtual "Device".
 * This could be something like a Switch, Lightbulb, etc.
 *
 * @note The device created needs to be added to a node using smart_home_node_add_device().
 *
 * @param[in] dev_name The unique device name.
 * @param[in] type Optional device type. Can be kept NULL.
 * @param[in] priv_data (Optional) Private data associated with the device. This will be passed to callbacks.
 * It should stay allocated throughout the lifetime of the device.
 *
 * @return Device handle on success.
 * @return NULL in case of any error.
 */
smart_home_device_t *smart_home_device_create(const char *dev_name, const char *type, void *priv_data);

/**
 * Add callbacks for a device/service
 *
 * Add read/write callbacks for a device that will be invoked as per requests received from the cloud (or other paths
 * as may be added in future).
 *
 * @param[in] device Device handle.
 * @param[in] write_cb Write callback.
 * @param[in] read_cb Read callback.
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t smart_home_device_add_cb(const smart_home_device_t *device, smart_home_device_write_cb_t write_cb, smart_home_device_read_cb_t read_cb);

/**
 * Add a device to a node
 *
 * @param[in] node Node handle.
 * @param[in] device Device handle.
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t smart_home_node_add_device(const smart_home_node_t *node, const smart_home_device_t *device);

/** Get device by name
 *
 * Get handle for a device based on the name.
 *
 * @param[in] node Node handle.
 * @param[in] device_name Device name to search.
 *
 * @return Device handle on success.
 * @return NULL in case of failure.
 */
smart_home_device_t *smart_home_node_get_device_by_name(const smart_home_node_t *node, const char *device_name);

/** Add a Device attribute
 *
 * @note Device attributes are reported only once after a boot-up as part of the node
 * configuration.
 * Eg. Serial Number
 * 
 * @param[in] device Device handle.
 * @param[in] attr_name Name of the attribute.
 * @param[in] val Value of the attribute.
 *
 * @return ESP_OK if the attribute was added successfully.
 * @return error in case of failure.
 */
esp_err_t smart_home_device_add_attribute(const smart_home_device_t *device, const char *attr_name, const char *val);

/** Get device name from handle
 *
 * @param[in] device Device handle.
 *
 * @return NULL terminated device name string on success.
 * @return NULL in case of failure.
 */
char *smart_home_device_get_name(const smart_home_device_t *device);

/** Get device type from handle
 *
 * @param[in] device Device handle.
 *
 * @return NULL terminated device type string on success.
 * @return NULL in case of failure.
 */
char *smart_home_device_get_type(const smart_home_device_t *device);

/**
 * Add a parameter to a device/service
 *
 * @param[in] device Device handle.
 * @param[in] param Parameter handle.
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t smart_home_device_add_param(const smart_home_device_t *device, const smart_home_param_t *param);

/** Get parameter by name
 *
 * Get handle for a parameter based on the name.
 *
 * @param[in] device Device handle.
 * @param[in] param_name Parameter name to search.
 *
 * @return Parameter handle on success.
 * @return NULL in case of failure.
 */
smart_home_param_t *smart_home_device_get_param_by_name(const smart_home_device_t *device, const char *param_name);

/**
 * Create a Parameter
 *
 * Parameter can be something like Temperature, Outlet state, Lightbulb brightness, etc.
 *
 * Any changes should be reported using the smart_home_param_update_and_report() API.
 * Any remote changes will be reported to the application via the device callback, if registered.
 *
 * @note The parameter created needs to be added to a device using smart_home_device_add_param().
 * Parameter name should be unique in a given device.
 *
 * @param[in] param_name Name of the parameter.
 a* @param[in] type Optional parameter type. Can be kept NULL.
 * @param[in] val Value of the parameter. This also specifies the type that will be assigned
 * to this parameter. You can use smart_home_bool(), smart_home_int(), smart_home_float()
 * or smart_home_str() functions as the argument here. Eg, smart_home_bool(true).
 * @param[in] properties Properties of the parameter, which will be a logical OR of flags in
 *  \ref smart_home_param_property_flags_t.
 *
 * @return Parameter handle on success.
 * @return NULL in case of failure.
 */
smart_home_param_t *smart_home_param_create(const char *param_name, const char *type,
        smart_home_param_val_t val, uint8_t properties);

/**
 * Add a UI Type to a parameter
 *
 * This will be used by the Phone apps (or other clients) to render appropriate UI for the given
 * parameter.
 *
 * @param[in] param Parameter handle.
 * @param[in] ui_type String describing the UI Type.
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t smart_home_param_add_ui_type(const smart_home_param_t *param, const char *ui_type);

/**
 * Add bounds for an integer/float parameter
 *
 * This can be used to add bounds (min/max values) for a given integer parameter. Eg. brightness
 * will have bounds as 0 and 100 if it is a percentage.
 * Eg. smart_home_param_add_bounds(brightness_param, smart_home_int(0), smart_home_int(100), smart_home_int(5));
 *
 * @note The SDK does not check the bounds. It is upto the application to handle it.
 *
 * @param[in] param Parameter handle.
 * @param[in] min Minimum allowed value.
 * @param[in] max Maximum allowed value.
 * @param[in] step Minimum stepping (set to 0 if no specific value is desired).
 *
 * @return ESP_OK on success.
 * return error in case of failure.
 */
esp_err_t smart_home_param_add_bounds(const smart_home_param_t *param,
    smart_home_param_val_t min, smart_home_param_val_t max, smart_home_param_val_t step);

/**
 * Add a list of valid strings for a string parameter
 *
 * This can be used to add a list of valid strings for a given string parameter.
 *
 * Eg.
 * static const char *valid_strs[] = {"None","Yes","No","Can't Say"};
 * smart_home_param_add_valid_str_list(param, valid_strs, 4);
 *
 * @note The RainMaker core does not check the values. It is upto the application to handle it.
 *
 * @param[in] param Parameter handle.
 * @param[in] strs Pointer to an array of strings. Note that this memory should stay allocated
 * throughout the lifetime of this parameter.
 * @param[in] count Number of strings in the above array.
 *
 * @return ESP_OK on success.
 * return error in case of failure.
 */
esp_err_t smart_home_param_add_valid_str_list(const smart_home_param_t *param, const char *strs[], uint8_t count);

/** Update and report a parameter
 *
 * Calling this API will update the parameter and report it to the cloud.
 * This should be used whenever there is any local change.
 *
 * @param[in] param Parameter handle.
 * @param[in] val New value of the parameter.
 *
 * @return ESP_OK if the parameter was updated successfully.
 * @return error in case of failure.
 */
esp_err_t smart_home_param_update_and_report(const smart_home_param_t *param, smart_home_param_val_t val);

/** Update and report a parameter
 *
 * Calling this API will update the parameter.
 *
 * @param[in] param Parameter handle.
 * @param[in] val New value of the parameter.
 *
 * @return ESP_OK if the parameter was updated successfully.
 * @return error in case of failure.
 */
esp_err_t smart_home_param_update(const smart_home_param_t *param, smart_home_param_val_t val);

/** Get parameter name from handle
 *
 * @param[in] param Parameter handle.
 *
 * @return NULL terminated parameter name string on success.
 * @return NULL in case of failure.
 */
char *smart_home_param_get_name(const smart_home_param_t *param);

/** Get parameter type from handle
 *
 * @param[in] param Parameter handle.
 *
 * @return NULL terminated parameter type string on success.
 * @return NULL in case of failure.
 */
char *smart_home_param_get_type(const smart_home_param_t *param);

/** Get parameter val from handle
 *
 * @param[in] param Parameter handle.
 *
 * @return Current parameter val on success.
 * @return Parameter val with INVALID type in case of failure.
 */
smart_home_param_val_t smart_home_param_get_val(const smart_home_param_t *param);

/** Initialise Smart Home
 *
 * This API initialises the node.
 *
 * @return ESP_OK in case of success.
 * @return error in case of failure.
 */
esp_err_t smart_home_init();

/** Print details
 *
 * This API prints the details of devices and params. It can be used for debugging.
 */
void smart_home_print_details();

#endif /* _SMART_HOME_H_ */
