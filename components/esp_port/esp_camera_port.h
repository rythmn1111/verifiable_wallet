#pragma once

#include <stdbool.h>
#include "esp_camera.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

void esp_camera_port_init(i2c_port_num_t i2c_port);
/** C-callable: init camera (pass 0 for default I2C port). Returns 0 on success. */
int esp_camera_port_init_c(int i2c_port_num);
/** True if camera was successfully initialized (e.g. at boot or when opening Sign Tx). */
bool esp_camera_port_is_inited(void);

#ifdef __cplusplus
}
#endif