#ifndef _LOG_H
#define _LOG_H

#include "headers.h"

int insert_in_log(int type, int ss_id, int ns_port, int ss_or_client_port, int request_type, char* path, int isAck);

#endif