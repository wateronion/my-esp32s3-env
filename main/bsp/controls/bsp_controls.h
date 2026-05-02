#pragma once

#include "esp_err.h"
#include <stdbool.h>

#define AC_GPIO             21
#define LIGHT_GPIO          47
#define CURTAIN_OPEN_GPIO   40
#define CURTAIN_CLOSE_GPIO  41

esp_err_t bsp_controls_init(void);

bool bsp_ac_get_state(void);
void bsp_ac_toggle(void);

bool bsp_light_get_state(void);
void bsp_light_toggle(void);

void bsp_curtain_open(void);
void bsp_curtain_close(void);
bool bsp_curtain_is_open(void);
