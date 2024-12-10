// client.c
#include "headers.h"
#include "client.h"

// Global variables for persistent connections
ServerInfo current_server;
int naming_server_socket = -1;

int main(int argc, char *argv[]) {
   // usage: ./client <naming_server_ip> <naming_server_port>
   if (argc != 3) {
      printf(RED"\nUsage: %s <naming_server_ip> <naming_server_port>\n"RESET, argv[0]);
      return 1;
   }

   // Connect to the naming server
   char* nm_ip = argv[1];
   int nm_port = atoi(argv[2]);

   int nm_socket = connect_to_naming_server(nm_ip, nm_port);

   // Initial connection to the naming server
   if ( nm_socket < 0) {
      // printf("\nUnable to connect to naming server\n");
      printf(RED"\nUnable to connect to naming server\n"RESET);
      return 1;
   }

   // printf("Connected to naming server\n");
   printf(YELLOW"Connected to naming server\n"RESET);

   // Main loop to handle user commands
   while (1) {
      printf("\nEnter command: ");
      char line[BUFFER_SIZE];
      char command[32];
      char path[MAX_PATH_LENGTH];
      char data[BUFFER_SIZE]; // Buffer to hold data for WRITE command
      char flag[8];           // Buffer for flags like -f or -d
      if (fgets(line, BUFFER_SIZE, stdin) == NULL) break;

      command[0] = '\0';
      path[0] = '\0';
      data[0] = '\0';
      flag[0] = '\0';

      // Parsing the input line
      sscanf(line, "%s", command);
      if (strlen(command) == 0)
         continue;

      if (strcmp(command, "EXIT") == 0) {
         break;
      }

      // Handle READ command
      if (strcmp(command, "READ") == 0) {
         // Extract the path from the input line
         if (sscanf(line, "%*s %s", path) < 1){
            printf("ERROR: READ command requires at least 2 arguments (READ <path>)\n");
            continue;
         }
         // Get storage server details first
         ServerInfo storage_server = get_storage_server(nm_ip, nm_port, path, nm_socket);
         if (storage_server.socket == -1) {
            printf("Failed to get storage server details\n");
            continue;
         }
         printf("port: %d, ip: %s\n", storage_server.port, storage_server.ip);
         int server_socket = connect_to_storage_server(&storage_server);
         if (server_socket == -1) {
            printf("Failed to connect to storage server\n");
            continue;
         }
         else{
            printf("Connected to storage server\n");
         }
         read_file(server_socket, path);
      } 
      else if (strcmp(command, "WRITE") == 0) {
         if (sscanf(line, "%*s %s %[^\n]", path, data) < 2){
            printf("ERROR: WRITE command requires at least 3 arguments (WRITE <path> <data>)\n");
            continue;
         }
         // Get storage server details first
         ServerInfo storage_server = get_storage_server(nm_ip, nm_port, path, nm_socket);
         if (storage_server.socket == -1) {
            printf("Failed to get storage server details\n");
            continue;
         }
         printf("port: %d, ip: %s\n", storage_server.port, storage_server.ip);
         int server_socket = connect_to_storage_server(&storage_server);
         if (server_socket == -1) {
            printf("Failed to connect to storage server\n");
            continue;
         }
         else{
            printf("Connected to storage server\n");
         }
         write_file(server_socket, path, data); // Assuming data is required for WRITE
      }
      else if (strcmp(command, "GET") == 0) { // get size and permissions
         if (sscanf(line, "%*s %s", path) < 2){
            printf("ERROR: GET command requires at least 2 arguments (WRITE <path>)\n");
            continue;
         }
         // Get storage server details first
         ServerInfo storage_server = get_storage_server(nm_ip, nm_port, path, nm_socket);
         if (storage_server.socket == -1) {
            printf("Failed to get storage server details\n");
            continue;
         }
         printf("port: %d, ip: %s\n", storage_server.port, storage_server.ip);
         int server_socket = connect_to_storage_server(&storage_server);
         if (server_socket == -1) {
            printf("Failed to connect to storage server\n");
            continue;
         }
         else{
            printf("Connected to storage server\n");
         }
         get_file_info(server_socket, path); // Assuming data is required for WRITE
      }
      else if (strcmp(command, "STREAM") == 0) {
         if (sscanf(line, "%*s %s", path) < 1){
            printf("ERROR: STREAM command requires at least 2 arguments (STREAM <path>)\n");
            continue;
         }
         ServerInfo storage_server = get_storage_server(nm_ip, nm_port, path, nm_socket);
         if (storage_server.socket == -1) {
            printf("Failed to get storage server details\n");
            continue;
         }
         int server_socket = connect_to_storage_server(&storage_server);
         if (server_socket == -1) {
            printf("Failed to connect to storage server\n");
            continue;
         }
         stream_audio_file_2(server_socket, path);
      }
      else if (strcmp(command, "CREATE") == 0){
         sscanf(line, "%*s %s %s %s", path, data, flag); // Extract path, name, and flag
         if (strlen(path) == 0 || strlen(data) == 0 || strlen(flag) == 0){
            printf("ERROR: CREATE command requires path, name, and flag (CREATE <path> <name> -f/-d)\n");
            continue;
         }
         if (strcmp(flag, "-f") != 0 && strcmp(flag, "-d") != 0){
            printf("ERROR: CREATE command requires a valid flag (-f or -d)\n");
            continue;
         }
         request_nm_create(nm_socket, path, data, flag);
      }
      else if (strcmp(command, "DELETE") == 0) {
         if (sscanf(line, "%*s %s %s", path, data) < 1){
            printf("ERROR: DELETE command requires at least 2 arguments (DELETE <path>)\n");
            continue;
         }
         request_nm_delete(nm_socket, path, data); // data is file name here
      } 
      else if (strcmp(command, "COPY") == 0) {
         if (sscanf(line, "%*s %s %s", path, data) < 2){
            printf("ERROR: COPY command requires at least 3 arguments (COPY <src> <dest>)\n");
            continue;
         }
         request_nm_copy(nm_socket, path, data); // data is dest, path is src here
      } 
      else {
         printf("Unknown command\n");
      }
      printf("restart while!!\n");
   }

   cleanup_connections();
   return 0;
}
