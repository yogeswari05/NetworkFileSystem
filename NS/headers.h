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
#include <stdbool.h>    

//MACROS
#define MAX_CACHE_SIZE 5
#define HASH_TABLE_SIZE 100         // Size of the hash map
#define MAX_STRING_SIZE 255
#define CHUNK_SIZE 32             // Fixed size chunks for flushing
#define LARGE_THRESHOLD 1024     // Threshold for large requests (1MB)

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

#define SS 0
#define CLIENT 1

#define CREATE_REQUEST 0
#define READ_REQUEST 1
#define WRITE_REQUEST 2
#define DELETE_REQUEST 3
#define COPY_REQUEST 4
#define STREAM_REQUEST 5

#define ACK 1
#define NACK 0





#endif