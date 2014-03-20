#ifndef PID_H
#define PID_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include "ch.h"
#include "types.h"
#include "sensor.h"


typedef enum {
  POSITIVE,
  NEGATIVE
};

typedef struct {
  float kp;
  float ki;
  float kd;

  float integral;
  float last_sample;
  float pid_output;

  bool   enabled;
  float  out_min;
  float  out_max;
  int8_t output_sign;

  /* Time is in system ticks */
  systime_t sample_time;
  systime_t last_time;
} pid_t;



void pid_init(pid_t* pid);
void pid_exec(pid_t* pid, quantity_t setpoint, quantity_t sample);
void pid_set_gains(pid_t* pid, float Kp, float Ki, float Kd);
void pid_enable(pid_t* pid, quantity_t sample, bool enabled);
void pid_reinit(pid_t* pid, quantity_t sample);
void pid_set_output_sign(pid_t* pid, uint8_t direction);
void pid_set_output_limits(pid_t* pid, float Min, float Max);

#endif
