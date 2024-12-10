#ifndef _HEADERS_H_
#define _HEADERS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <net/if.h>
#include <vlc/vlc.h>
#include <dirent.h>
#include <semaphore.h>

// MACROS
#define CHUNK_SIZE 8             // Fixed size chunks for flushing
#define LARGE_THRESHOLD 32     // Threshold for large requests (1MB)

#define MAX_DATA_SIZE 4096

#define RED             "\033[1;31m"
#define GREEN           "\033[1;32m"
#define YELLOW          "\033[1;33m"
#define BLUE            "\033[1;34m"
#define MAGENTA         "\033[1;35m"
#define CYAN            "\033[1;36m"
#define WHITE           "\033[1;37m"
#define ORANGE          "\033[1;91m"
#define PURPLE          "\033[1;95m"
#define PINK            "\033[1;95m"

#define RESET           "\033[0m"

#endif