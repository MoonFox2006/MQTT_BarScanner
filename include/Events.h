#ifndef __EVENTS_H
#define __EVENTS_H

#include "Queue.h"

struct __packed event_t {
  uint8_t id;
  uint8_t data;
};

typedef Queue<event_t, 32> EventQueue;

#endif
