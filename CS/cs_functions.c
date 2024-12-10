#include "headers.h"
#include "client.h"
#include "errors.h"

// Function to connect to naming server once
int connect_to_naming_server(const char* nm_ip, int nm_port) {
   if (naming_server_socket != -1) {
      return naming_server_socket;  // Return existing connection
   }
   naming_server_socket = socket(AF_INET, SOCK_STREAM, 0);
   struct sockaddr_in nm_addr;

   memset(&nm_addr, 0, sizeof(nm_addr));
   nm_addr.sin_family = AF_INET;
   nm_addr.sin_addr.s_addr = inet_addr(nm_ip);
   nm_addr.sin_port = htons(nm_port);

   if (connect(naming_server_socket, (struct sockaddr*)&nm_addr, sizeof(nm_addr)) < 0) {
      perror("Connection to naming server failed");
      naming_server_socket = -1;
      return -1;
   }
   // Identify as client (only once)
   const char* client_type = "CLIENT";
   send(naming_server_socket, client_type, strlen(client_type), 0);

   return naming_server_socket;
}

// Function to get storage server details from naming server
ServerInfo get_storage_server(const char* nm_ip, int nm_port, const char* path, int nm_socket) {

   ServerInfo server_info;
   memset(&server_info, 0, sizeof(ServerInfo));

   // Send path request to naming server
   char request[BUFFER_SIZE];
   sprintf(request, "GET_SERVER %s", path);
   send(nm_socket, request, strlen(request), 0);

   // Receive acknowledgment from naming server
   char ack[BUFFER_SIZE];
   memset(ack, 0, sizeof(ack));
   ssize_t bytes_received = recv(nm_socket, ack, BUFFER_SIZE - 1, 0);
   printf("Received acknowledgment: %s\n", ack);
   if(strcmp(ack, "ACK") == 0){
      printf("Received ACK from naming server\n");
   }
   else{
      printf("Failed to receive ACK from naming server\n");
   }

   // ssize_t bytes_sent = recv(nm_socket, request, BUFFER_SIZE - 1, 0);

   // struct timeval timeout;
   // timeout.tv_sec = 5;  // 5-second timeout
   // timeout.tv_usec = 0;
   // if (setsockopt(nm_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
   //    perror("Error setting socket timeout");
   //    return; // Or handle the error as appropriate for your function
   // }

   // ssize_t bytes_received = recv(nm_socket, ack, BUFFER_SIZE - 1, 0);
   // if (bytes_received > 0) {
   //    ack[bytes_received] = '\0'; // Null-terminate the received data
   //    if (strcmp(ack, "ACK") == 0) {
   //       printf("Received ACK from naming server\n");
   //    } else {
   //       printf("Unexpected message: %s\n", ack);
   //    }
   // } else if (bytes_received == 0) {
   //    printf("Connection closed by the naming server\n");
   // } else {
   //    if (errno == EWOULDBLOCK || errno == EAGAIN) {
   //       printf("Timeout: Failed to receive ACK from naming server\n");
   //    } else {
   //       perror("recv error");
   //    }
   // }

   // Receive server info from naming server
   char response[BUFFER_SIZE];
   memset(response, 0, sizeof(response));
   bytes_received = recv(nm_socket, response, BUFFER_SIZE - 1, 0);
   if (bytes_received <= 0) {
      // printf("Failed to receive response from naming server\n");
      printf(RED "File not found: %s\n" RESET, path);
      return server_info;
   }

   response[bytes_received] = '\0';

   if(strcmp(response, ERROR_FILE_NOT_ACCESSIBLE) == 0){
      printf(RED "File not found: %s\n" RESET, path);
      server_info.socket = -1;
      return server_info;
   }

   printf("(client)Received response: %s\n", response);
   sscanf(response, "%s %d", server_info.ip, &server_info.port);
   return server_info;
}

// Function to send the request and receive acknowledgment from naming server
void send_request_and_receive_ack(int nm_socket, const char* request, char* ack_buffer) {
   send(nm_socket, request, strlen(request), 0);
   printf("Sent request: %s\n", request);

   memset(ack_buffer, 0, BUFFER_SIZE);
   ssize_t bytes_received = recv(nm_socket, ack_buffer, BUFFER_SIZE - 1, 0);
   if (bytes_received > 0) {
      ack_buffer[bytes_received] = '\0';
      printf("(client) Received acknowledgment from naming server: %s\n", ack_buffer);
   } 
   else {
      printf("(client) Failed to receive acknowledgment from naming server\n");
   }
}

// Function for CREATE operation
void request_nm_create(int nm_socket, const char *path, const char *name, const char *flag){
   if (strcmp(flag, "-f") != 0 && strcmp(flag, "-d") != 0) {
      printf("ERROR: Invalid flag for CREATE operation. Use -f for file or -d for directory.\n");
      return;
   }
   // Send the create request to the naming server
   char request[BUFFER_SIZE];
   snprintf(request, sizeof(request), "CREATE %s %s %s", path, name, flag);
   char ack_buffer[BUFFER_SIZE];
   send_request_and_receive_ack(nm_socket, request, ack_buffer);
}

// Function for DELETE operation
void request_nm_delete(int nm_socket, const char *path, char* name){
   // Send the delete request to the naming server
   char request[BUFFER_SIZE];
   snprintf(request, sizeof(request), "DELETE %s %s", path, name);
   char ack_buffer[BUFFER_SIZE];
   send_request_and_receive_ack(nm_socket, request, ack_buffer);
}

// Function for COPY operation
void request_nm_copy(int nm_socket, const char *source_path, const char *dest_path){
   // Send the copy request to the naming server
   char request[BUFFER_SIZE];
   snprintf(request, sizeof(request), "COPY %s %s", source_path, dest_path);

   char ack_buffer[BUFFER_SIZE];
   send_request_and_receive_ack(nm_socket, request, ack_buffer);
}

// Cleanup connections before exit
void cleanup_connections() {
   if (naming_server_socket != -1) {
      close(naming_server_socket);
   }
   if (current_server.socket != -1) {
      close(current_server.socket);
   }
}

// Function to connect to the storage server
int connect_to_storage_server(ServerInfo *server){
   printf("Connecting to storage server %s:%d\n", server->ip, server->port);
   // Create socket for storage server connection
   server->socket = socket(AF_INET, SOCK_STREAM, 0);
   if (server->socket < 0){
      perror("Error creating socket for storage server");
      return -1;
   }

   struct sockaddr_in server_addr;
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = inet_addr(server->ip);
   server_addr.sin_port = htons(server->port);

   if (connect(server->socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
      perror("Error connecting to storage server");
      return -1;
   }
   return server->socket;
}

// Function to read the file from the storage server
void read_file(int server_socket, const char *file_path) {
   char operation[] = "READ";
   char message[256]; // Ensure the size is large enough for both
   snprintf(message, sizeof(message), "%s %s", operation, file_path);

   // Send the file path to the server to request the file
   if (send(server_socket, message, strlen(message) + 1, 0) < 0) {
      perror("Failed to send file path to server");
      close(server_socket);
      return;
   }
   else{
      printf("Requesting storage server to '%s' file: %s\n", operation, file_path);
   }

   // Buffer to receive the file content
   char buffer[BUFFER_SIZE];
   ssize_t bytes_received;

   // Receive the file content from the server
   memset(buffer, 0, sizeof(buffer));
   while ((bytes_received = recv(server_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
      buffer[bytes_received] = '\0'; // Null-terminate the received data for string operations

      // Check for the "END_OF_FILE" marker
      if (strstr(buffer, "END_OF_FILE") != NULL) {
         // Print up to the "END_OF_FILE" marker and break
         char *end_marker = strstr(buffer, "END_OF_FILE");
         fwrite(buffer, 1, end_marker - buffer, stdout);
         printf("\nFile transfer complete\n");
         break;
      }
      // Print the received data
      fwrite(buffer, 1, bytes_received, stdout);
   }
   if (bytes_received < 0) {
      perror("Error receiving file content");
   }
   close(server_socket);
}

void write_file(int server_socket, const char *file_path, const char *data) {
   // Send the file path to the server
   char operation[] = "WRITE";
   char message[1024]; // Adjust size if needed
   snprintf(message, sizeof(message), "%s %s %s", operation, file_path, data);

   // Send the file path to the server to request the file
   if (send(server_socket, message, strlen(message) + 1, 0) < 0){
      perror("Failed to send file path to server");
      close(server_socket);
      return;
   }
   else{
      printf("Requesting storage server to '%s' file: %s data: %s\n", operation, file_path, data);
   }
   // Check server's response
   char response[256];
   memset(response, 0, sizeof(response));
   ssize_t bytes_received = recv(server_socket, response, sizeof(response), 0);
   if (bytes_received > 0) {
      response[bytes_received] = '\0';
      printf("Server Response: %s\n", response);
   } 
   else {
      perror("Failed to receive server response");
   }
   close(server_socket);
}

void get_file_info(int server_socket, const char *file_path) {
   // Construct the request string with "GET" and the file path
   char request[512]; // Ensure this is large enough to hold "GET " + file_path
   snprintf(request, sizeof(request), "GET %s", file_path);

   // Send the constructed request to the server
   if (send(server_socket, request, strlen(request) + 1, 0) < 0) {
      perror("Failed to send request to server");
      close(server_socket);
      return;
   }
   // Receive file information from the server
   char response[256];
   memset(response, 0, sizeof(response));
   ssize_t bytes_received = recv(server_socket, response, sizeof(response), 0);
   if (bytes_received > 0) {
      response[bytes_received] = '\0';
      printf("File Info: %s\n", response);
   } 
   else {
      perror("Failed to receive file info");
   }
   close(server_socket);
}

void stream_audio_file(int server_socket, const char *file_path) {
   // Construct the request string with "STREAM" and the file path
   char request[512];
   snprintf(request, sizeof(request), "STREAM %s", file_path);

   // Send the request to the server
   if (send(server_socket, request, strlen(request) + 1, 0) < 0) {
      perror("Failed to send request to server");
      close(server_socket);
      return;
   }

   // Use a specific audio output device if the default fails
   char command[1024];
   snprintf(command, sizeof(command), "mpv --no-video --audio-device=alsa/default %s", file_path);

   // Check if the system has a default device
   if (system(command) == -1) {
      perror("Failed to start audio player");
   }

   close(server_socket);
}

void stream_audio_file_2(int server_socket, const char *file_path) {

   char request[512];
   snprintf(request, sizeof(request), "STREAM %s", file_path);

   // Send the request to the server
   if (send(server_socket, request, strlen(request) + 1, 0) < 0) {
      perror("Failed to send request to server");
      close(server_socket);
      return;
   }

   char buffer[BUFFER_SIZE];
   FILE *pipe;

   // Open a pipe to `mpv` to stream audio dynamically
   pipe = popen("mpv --no-video - --audio-device=alsa/default", "w");
   if (!pipe) {
      perror("Failed to start audio player");
      close(server_socket);
      return;
   }

   // Receive audio data from the server and write it to the player
   ssize_t bytes_received;
   memset(buffer, 0, sizeof(buffer));
   while ((bytes_received = recv(server_socket, buffer, BUFFER_SIZE, 0)) > 0) {
      // Write received data to the player's stdin
      if(strcmp(buffer, ERROR_INCORRECT_AUDIO_FORMAT) == 0){
         printf(RED"Audio format is incorrect\n"RESET);
         break;
      }
      if (fwrite(buffer, 1, bytes_received, pipe) != (size_t)bytes_received) {
         perror("Error writing to audio player");
         break;
      }
   }

   if (bytes_received < 0) {
      perror("Error receiving data from server");
   }

   // Close the player and the socket
   pclose(pipe);
   close(server_socket);
}
