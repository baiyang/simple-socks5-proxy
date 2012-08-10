#ifndef _WORK_H
#define _WORK_H

#include <pthread.h>
#include "socks.h"

extern Socket _socks_server;

bool init();
void serve_forever();
void shutdown();

#endif // endif 