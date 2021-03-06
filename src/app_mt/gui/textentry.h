
#ifndef GUI_TEXTENTRY_H
#define GUI_TEXTENTRY_H

#include "widget.h"

typedef enum {
  TXT_FMT_ANY,
  TXT_FMT_IP,
} textentry_format_t;

typedef void (*text_handler_t)(const char* text, void* user_data);

void
textentry_screen_show(textentry_format_t format, text_handler_t text_handler, void* user_data);

#endif
