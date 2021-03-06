
#ifndef GUI_QUANTITY_SELECT_H
#define GUI_QUANTITY_SELECT_H

#include "widget.h"

typedef void (*quantity_select_cb_t)(quantity_t quantity, void* user_data);

widget_t*
quantity_select_screen_create(
    const char* title,
    quantity_t quantity,
    float min_value,
    float max_value,
    float* velocity_steps,
    uint8_t num_velocity_steps,
    quantity_select_cb_t cb,
    void* cb_data);

#endif
