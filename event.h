#ifndef EVENT_H
#define EVENT_H

#define EVENT_NONE  0
#define EVENT_DEBUG 1

#define EVENT_PRESS   0
#define EVENT_RELEASE 1

void event_init();
void event_exit();
int event_main();

#endif
