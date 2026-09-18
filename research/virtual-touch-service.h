#ifndef A19_VIRTUAL_TOUCH_SERVICE_H
#define A19_VIRTUAL_TOUCH_SERVICE_H
#include <stdint.h>
uint64_t virtual_touch_start(void);
int virtual_touch_dispatch(void *event);
void virtual_touch_stop(void);
#endif
