
#include "quantity_widget.h"
#include "label.h"
#include "gfx.h"
#include "message.h"
#include "sensor.h"
#include "gui.h"
#include "app_cfg.h"

#include <stdio.h>
#include <string.h>


#define SPACE 8


typedef struct {
  quantity_t sample;
  widget_t* widget;
} quantity_widget_t;


static void quantity_widget_destroy(widget_t* w);
static void quantity_widget_paint(paint_event_t* event);


static const widget_class_t quantity_widget_class = {
    .on_destroy = quantity_widget_destroy,
    .on_paint   = quantity_widget_paint,
};

widget_t*
quantity_widget_create(widget_t* parent, rect_t rect, unit_t display_unit)
{
  quantity_widget_t* s = calloc(1, sizeof(quantity_widget_t));

  rect.height = font_opensans_regular_62->line_height;
  s->widget = widget_create(parent, &quantity_widget_class, s, rect);

  s->sample.unit = display_unit;
  s->sample.value = NAN;

  return s->widget;
}

static void
quantity_widget_destroy(widget_t* w)
{
  quantity_widget_t* s = widget_get_instance_data(w);

  free(s);
}

static void
quantity_widget_paint(paint_event_t* event)
{
  quantity_widget_t* s = widget_get_instance_data(event->widget);
  rect_t rect = widget_get_rect(event->widget);

  char* unit_str;
  switch (s->sample.unit) {
  case UNIT_TEMP_DEG_C:
    unit_str = "C";
    break;

  case UNIT_TEMP_DEG_F:
    unit_str = "F";
    break;

  case UNIT_TIME_SEC:
    unit_str = "sec";
    break;

  case UNIT_TIME_MIN:
    unit_str = "min";
    break;

  case UNIT_TIME_HOUR:
    unit_str = "hr";
    break;

  case UNIT_TIME_DAY:
    unit_str = "day";
    break;

  case UNIT_NONE:
  default:
    unit_str = "";
    break;
  }

  char value_str[16];
  if (isnan(s->sample.value))
    strncpy(value_str, "--.-", sizeof(value_str));
  else
    snprintf(value_str, sizeof(value_str), "%d.%d",
        (int)(s->sample.value),
        ((int)(fabs(s->sample.value) * 10.0f)) % 10);

  Extents_t value_ext = font_text_extents(font_opensans_regular_62, value_str);
  Extents_t unit_ext = font_text_extents(font_opensans_regular_22, unit_str);

  point_t center = rect_center(rect);

  int value_x = center.x - ((value_ext.width + SPACE + unit_ext.width) / 2);

  gfx_set_fg_color(WHITE);
  gfx_set_font(font_opensans_regular_62);
  gfx_draw_str(value_str, -1, value_x, rect.y);

  gfx_set_fg_color(DARK_GRAY);
  gfx_set_font(font_opensans_regular_22);
  gfx_draw_str(unit_str, -1, value_x + value_ext.width + SPACE, rect.y);
}

void
quantity_widget_set_value(widget_t* w, quantity_t sample)
{
  quantity_widget_t* s = widget_get_instance_data(w);

  // ensure that the given quantity is in the correct display unit
  sample = quantity_convert(sample, s->sample.unit);

  if (s->sample.value != sample.value) {
    s->sample.value = sample.value;
    widget_invalidate(s->widget);
  }
}

void
quantity_widget_set_unit(widget_t* w, unit_t unit)
{
  quantity_widget_t* s = widget_get_instance_data(w);

  if (s->sample.unit != unit) {
    s->sample.unit = unit;
    widget_invalidate(s->widget);
  }
}
