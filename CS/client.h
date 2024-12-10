#ifndef CLIENT_H
#define CLIENT_H

#include "headers.h"

#define BUFFER_SIZE 4096
#define MAX_PATH_LENGTH 256

typedef struct {
   char ip[16];
   int port;
   int socket;  // Store the socket connection to the storage server
} ServerInfo;

extern ServerInfo current_server;
extern int naming_server_socket;

int connect_to_naming_server(const char* nm_ip, int nm_port);
ServerInfo get_storage_server(const char* nm_ip, int nm_port, const char* path, int nm_socket);
void send_request_and_receive_ack(int nm_socket, const char* request, char* ack_buffer);
void read_file(int server_socket, const char* file_path);
void write_file(int server_socket, const char* file_path, const char* data);
void get_file_info(int server_socket, const char* file_path);
void stream_audio_file_2(int server_socket, const char* file_path);
void request_nm_create(int nm_socket, const char* path, const char* data, const char* flag);
void request_nm_delete(int nm_socket, const char* path, char* name);
void request_nm_copy(int nm_socket, const char* source_path, const char* dest_path);
void cleanup_connections();
int connect_to_storage_server(ServerInfo* server);

#endif