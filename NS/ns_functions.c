#include "namingServer.h"
#include "headers.h"
#include "helper.h"
#include "cache2.h"
#include "log.h"
#include "errors.h"

// Hash function

NamingServer naming_server;   

unsigned int hash(const char *key) {
   unsigned int hash = 0;
   while (*key) {
      hash = (hash * 31) + *key++;
   }
   return hash % HASH_TABLE_SIZE;
}

// Initialize the hash map
void initialize_hash_map(HashMap *map) {
   for (int i = 0; i < HASH_TABLE_SIZE; i++) {
      map->table[i] = NULL;
   }
   pthread_mutex_init(&map->lock, NULL);
}

// Add a path-to-server mapping
void hash_map_insert(HashMap *map, const char *path, StorageServer *server) {
   unsigned int index = hash(path);

   pthread_mutex_lock(&map->lock);
   HashNode *new_node = malloc(sizeof(HashNode));
   strcpy(new_node->path, path);
   new_node->server = server;
   new_node->next = map->table[index];
   map->table[index] = new_node;
   pthread_mutex_unlock(&map->lock);
}

// Find a storage server by path
StorageServer* hash_map_find(HashMap *map, const char *path) {
   printf("Finding path in hash map: %s\n", path);
   unsigned int index = hash(path);

   pthread_mutex_lock(&map->lock);
   HashNode *current = map->table[index];
   while (current) {
      if (strcmp(current->path, path) == 0) {
         pthread_mutex_unlock(&map->lock);
         return current->server;
      }
      current = current->next;
   }
   pthread_mutex_unlock(&map->lock);
   printf("Path not found in hash_map_find\n");
   return NULL; // Path not found
}

// Function to print the entire hash map
void hash_map_print(HashMap *map) {
   pthread_mutex_lock(&map->lock);

   // printf("Hash Map Contents:\n");
   printf(GREEN"Hash Map Contents:\n");
   for (int i = 0; i < HASH_TABLE_SIZE; i++) {
      HashNode *current = map->table[i];
      while (current != NULL) {
         printf("Path: %s, Server: %s:%d\n", current->path,
                  current->server->ip_address, current->server->client_port);
         current = current->next;
      }
   }
   printf(RESET);

   pthread_mutex_unlock(&map->lock);
}

void handle_storage_server_registration(int client_socket) {
   printf("Storage Server registration initiated\n");
   char buffer[BUFFER_SIZE];
   StorageServer new_ss;
   new_ss.num_paths = 0;
   new_ss.socket = client_socket;
   // Receive storage server details
   memset(buffer, 0, BUFFER_SIZE);
   ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
   if (bytes_received > 0) {
      buffer[bytes_received] = '\0';

      // Parse registration message
      char paths_str[BUFFER_SIZE];
      int num_paths;

      // Parse the basic information (IP, nm_port, client_port, num_paths)
      // Parse the basic information (IP, nm_port, server_port, client_port, num_paths)
      int parsed_fields = sscanf(buffer, "%s %d %d %d %d %[^\n]", 
         new_ss.ip_address, 
         &new_ss.nm_port,  
         &new_ss.server_port, 
         &new_ss.client_port, 
         &num_paths, 
         paths_str);

      if (parsed_fields < 5) {
         const char *error = "Invalid registration format";
         send(client_socket, error, strlen(error), 0);
         return;
      }

       // Parse accessible paths (space-separated, not comma-separated)
      char *path = strtok(paths_str, " ");
      while (path != NULL && new_ss.num_paths < MAX_ACCESSIBLE_PATHS) {
         strncpy(new_ss.accessible_paths[new_ss.num_paths], path, MAX_PATH_LENGTH - 1);
         new_ss.accessible_paths[new_ss.num_paths][MAX_PATH_LENGTH - 1] = '\0';  // Null-terminate
         new_ss.num_paths++;
         path = strtok(NULL, " ");
      }

      new_ss.socket = client_socket;
      new_ss.is_active = 1;

      // Add to storage servers list
      pthread_mutex_lock(&naming_server.lock);
      if (naming_server.num_storage_servers < MAX_STORAGE_SERVERS) {
         naming_server.storage_servers[naming_server.num_storage_servers++] = new_ss;
         printf("Storage Server registered: %s:%d\n", new_ss.ip_address, new_ss.nm_port);
         printf(YELLOW"Accessible paths:\n");
         for (int i = 0; i < new_ss.num_paths; i++) {
            printf("  %s\n", new_ss.accessible_paths[i]);
         }
         printf(RESET);
         // Add paths to hash map
         for (int i = 0; i < new_ss.num_paths; i++){
            hash_map_insert(&naming_server.path_to_server_map,
                            new_ss.accessible_paths[i],
                            &naming_server.storage_servers[naming_server.num_storage_servers - 1]);
         }

         printf("Storage Server registered: %s:%d\n", new_ss.ip_address, new_ss.nm_port);

         // Send acknowledgment
         const char *ack = "Registration successful";
         send(client_socket, ack, strlen(ack), 0);
      } 
      else {
         const char *error = "Maximum number of storage servers reached";
         send(client_socket, error, strlen(error), 0);
      }
      pthread_mutex_unlock(&naming_server.lock);
      // Print the entire hash map to verify
      hash_map_print(&naming_server.path_to_server_map);
   } 
   else {
      perror("Failed to receive registration message");
   }
}

void handle_client_request(int client_socket) {
   printf("Client request\n");
   char buffer[BUFFER_SIZE];
   
   // Receive client request  
   while (1) {
      memset(buffer, 0, BUFFER_SIZE);
      ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
      
      if (bytes_received <= 0) {
         break;
      }      

      // log that client has requested
      // log_message(buffer, );
      struct sockaddr_in client_addr;
      socklen_t addr_len = sizeof(client_addr);
      getpeername(client_socket, (struct sockaddr *)&client_addr, &addr_len);
      const char *ip = inet_ntoa(client_addr.sin_addr);
      int port = ntohs(client_addr.sin_port);

      log_message(buffer, ip, port);

      buffer[bytes_received] = '\0';

      char* Ack = "ACK";
      if(send(client_socket, Ack, strlen(Ack), 0) < 0){
         perror("Failed to send ACK to client");
      }
      else{
         printf("Sent ACK to client\n");
      }

      usleep(100);
      

      // printf(" buffer: %s\n",buffer);
      
      // Parse client request
      char command1[32];
      char path[MAX_PATH_LENGTH];
      sscanf(buffer, "%s %s", command1, path);   
      printf("Command: %s, Path: %s\n", command1, path);  
      // Check if the command is GET_SERVER 
      if (strcmp(command1, "GET_SERVER") == 0) { // READ, WRITE, GET(file info, permissions), STREAM
         printf("\nentered\n");
         // Find appropriate storage server
         pthread_mutex_lock(&naming_server.lock);
         // search in cache first

         StorageServer *server = NULL;
         server = access_cache(path);
         // server = hash_map_find(&naming_server.path_to_server_map, path);

         if (server) {
            printf("storage Server found\n");
            char response[BUFFER_SIZE];
            sprintf(response, "%s %d", server->ip_address, server->client_port);
            if(send(client_socket, response, strlen(response), 0) < 0){
               perror("Failed to send server info to client");
            }
            else{
               printf("Sent server info to client\n");
               printf("Server info: %s %d\n", server->ip_address, server->client_port);
            }
         } 
         else {
            printf("No storage server found\n");
            // const char *error = "No server found for the requested path";
            // send(client_socket, error, strlen(error), 0);
            char response[BUFFER_SIZE];
            sprintf(response, "%s", ERROR_FILE_NOT_ACCESSIBLE);
            printf("sending error code to client\n");
            if(send(client_socket, response, strlen(response), 0) < 0){
               perror("Failed to send server info to client");
            }
            else{
               printf("sent error code successfully\n");
            }
         }
         pthread_mutex_unlock(&naming_server.lock);
      }
      else { // CREATE, DELETE, COPY
         char command[50];
         char path1[MAX_PATH_LENGTH] = {0};
         char path2[MAX_PATH_LENGTH] = {0};
         char name[MAX_PATH_LENGTH] = {0}; // Variable to store the name for CREATE/DELETE
         char flag[MAX_ACCESSIBLE_PATHS] = {0};              // Flag for CREATE (-f or -d)

         // Parse the command
         char *token = strtok(buffer, " ");
         if (token) {
            strncpy(command, token, sizeof(command) - 1);
         } 
         else {
            const char *error = "Invalid command";
            send(client_socket, error, strlen(error), 0);
            continue;
         }
         pthread_mutex_lock(&naming_server.lock);
         StorageServer *server = hash_map_find(&naming_server.path_to_server_map, path);
         if(!server){
            const char *error = "No server found for the requested path";
            send(client_socket, error, strlen(error), 0);
            pthread_mutex_unlock(&naming_server.lock);
            continue;
         }
         if (strcmp(command, "CREATE") == 0) {

            // Expecting two tokens for CREATE (path and name)
            token = strtok(NULL, " ");
            if (token) {
               strncpy(path1, token, sizeof(path1) - 1);
            }
            token = strtok(NULL, " ");
            if (token) {
               strncpy(name, token, sizeof(name) - 1);
            }
            token = strtok(NULL, " ");
            if (token) {
               strncpy(flag, token, sizeof(flag) - 1);
            }
            printf("File Path: %s\n", path1);
            printf("File Name: %s\n", name);
            printf("Flag: %s\n", flag);

            // if(insert_in_log(CLIENT, 0, 0, server->client_port, CREATE_REQUEST, path, 0) == 1){
            //    printf("Inserted create in log\n");
            // }


            // Check if both path and name are provided
            if (strlen(path1) == 0 || strlen(name) == 0 || strlen(flag) == 0) {
               const char *error = "Invalid CREATE command format. Expected: CREATE <path> <name> -f/-d";
               send(client_socket, error, strlen(error), 0);
               pthread_mutex_unlock(&naming_server.lock);
               continue;
            }
            if (strcmp(flag, "-f") != 0 && strcmp(flag, "-d") != 0) {
               const char *error = "Invalid CREATE flag. Use -f for file or -d for directory.";
               send(client_socket, error, strlen(error), 0);
               pthread_mutex_unlock(&naming_server.lock);
               continue;
            }
            // printf("Handling CREATE request for: %s with name: %s\n", path1, name);
            char request[BUFFER_SIZE];
            snprintf(request, sizeof(request), "CREATE %s %s %s", path1, name, flag);
            send(server->socket, request, strlen(request), 0);
            printf("Sent CREATE to storage server: %s\n", request);

            char storage_server_ack[BUFFER_SIZE];
            memset(storage_server_ack, 0, sizeof(storage_server_ack));
            ssize_t ack_received = recv(server->socket, storage_server_ack, sizeof(storage_server_ack), 0);
            if (ack_received > 0) {
               storage_server_ack[ack_received] = '\0';
               printf("Storage server acknowledgment: %s\n", storage_server_ack);

               log_message(storage_server_ack, server->ip_address, server->client_port);

               // Send this acknowledgment to the client
               send(client_socket, storage_server_ack, strlen(storage_server_ack), 0);
            } 
            else {
               const char *error = "Failed to receive acknowledgment from storage server";
               send(client_socket, error, strlen(error), 0);
            }
         }
         else if (strcmp(command, "DELETE") == 0) {
            // Expecting two tokens for DELETE (path and name)
            token = strtok(NULL, " ");
            if (token) {
               strncpy(path1, token, sizeof(path1) - 1);
            }
            token = strtok(NULL, " ");
            if (token) {
               strncpy(name, token, sizeof(name) - 1);
            }
            // Check if both path and name are provided
            if (strlen(path1) == 0 || strlen(name) == 0) {
               const char *error = "Invalid DELETE command format. Expected: DELETE <path> <name>";
               send(client_socket, error, strlen(error), 0);
               pthread_mutex_unlock(&naming_server.lock);
               continue;
            }

            // if(insert_in_log(CLIENT, 0, 0, server->client_port, DELETE_REQUEST, path, 0) == 1){
            //    printf("Inserted delete in log\n");
            // }

            printf("Handling DELETE request for: %s with name: %s\n", path1, name);
            char request[BUFFER_SIZE];
            snprintf(request, sizeof(request), "DELETE %s %s", path1, name);
            send(server->socket, request, strlen(request), 0);
            printf("Sent DELETE to storage server: %s\n", request);
            memset(buffer, 0, sizeof(buffer));
            recv(server->socket, buffer, sizeof(buffer) - 1, 0);
            printf("Received response from storage server: %s\n", buffer);
            log_message(buffer, server->ip_address, server->client_port);
         }
         else if (strcmp(command, "COPY") == 0) {
            // Expecting two tokens for COPY (source path and destination path)
            token = strtok(NULL, " ");
            if (token) {
               strncpy(path1, token, sizeof(path1) - 1);
            }
            token = strtok(NULL, " ");
            if (token) {
               strncpy(path2, token, sizeof(path2) - 1);
            }
            // Check if both paths are provided
            if (strlen(path1) == 0 || strlen(path2) == 0) {
               const char *error = "Invalid COPY command format. Expected: COPY <source_path> <destination_path>";
               send(client_socket, error, strlen(error), 0);
               pthread_mutex_unlock(&naming_server.lock);
               continue;
            }

            // if(insert_in_log(CLIENT, 0, 0, server->client_port, COPY_REQUEST, path, 0) == 1){
            //    printf("Inserted copy in log\n");
            // }

            // Retrieve the storage server associated with the source and destination paths
            StorageServer *source_ss = hash_map_find(&naming_server.path_to_server_map, path1);
            StorageServer *dest_ss = hash_map_find(&naming_server.path_to_server_map, path2);
            // Check if the source and destination paths exist in the hashmap
            if (!source_ss) {
               const char *error = "Source path not found";
               send(client_socket, error, strlen(error), 0);
               pthread_mutex_unlock(&naming_server.lock);
               continue;
            }
            if (!dest_ss) {
               const char *error = "Destination path not found";
               send(client_socket, error, strlen(error), 0);
               pthread_mutex_unlock(&naming_server.lock);
               continue;
            }
            printf("Handling COPY request for: %s to %s on different storage servers\n", path1, path2);

            // Prepare and send SEND request to source storage server
            char request[BUFFER_SIZE];
            snprintf(request, sizeof(request), "SEND %s", path1); // SEND request for the entire directory or file
            send(source_ss->socket, request, strlen(request), 0);
            printf("Sent SEND request to source storage server: %s\n", request);
            printf("request: %s\n", request);

            // Prepare and send RECEIVE request to destination storage server
            snprintf(request, sizeof(request), "RECEIVE %s", path2); // RECEIVE request with the destination folder path
            send(dest_ss->socket, request, strlen(request), 0);
            printf("Sent RECEIVE request to destination storage server: %s\n", request);

            // Optionally, handle receiving data from destination server
            char buffer[BUFFER_SIZE];
            ssize_t bytes_received;
            memset(buffer, 0, sizeof(buffer));
            while ((bytes_received = recv(source_ss->socket, buffer, sizeof(buffer) - 1, 0)) > 0){
               printf("bytes_received: %ld\n", bytes_received);
               printf("buffer: [%s]\n", buffer);
               if(strstr(buffer, "END_OF_DATA") != NULL){
                  break;
               }
               buffer[bytes_received] = '\0';
               printf("Forwarding data...: %s\n", buffer);
               // Forward to destination storage server
               send(dest_ss->socket, buffer, bytes_received, 0);
               memset(buffer, 0, sizeof(buffer));
               printf("end of while\n");
            }

            printf("Data forwarding complete\n");
            // Send end marker to destination server
            char* end_marker = "END_OF_DATA";
            sleep(1);
            // send(handler->nm_socket, "END_OF_DATA", strlen("END_OF_DATA"), 0);
            if(send(dest_ss->socket, end_marker, strlen(end_marker)+1, 0) < 0){
               perror(RED"Failed to send end marker"RESET);
            }
            else{
               printf("End marker sent to Naming Server.\n");
            }

            if (bytes_received < 0){
               perror("Error while receiving data");
            } // Notify the client when the copy operation is complete

            char ack_buffer[BUFFER_SIZE];
            memset(ack_buffer, 0, sizeof(ack_buffer));
            ssize_t ack_received = recv(dest_ss->socket, ack_buffer, sizeof(ack_buffer) - 1, 0);
            if (ack_received > 0){
               ack_buffer[ack_received] = '\0';
               // Log acknowledgment from destination server
               log_message(ack_buffer, dest_ss->ip_address, dest_ss->client_port);
               // printf("ACK from destination server logged: %s\n", ack_buffer);
            }
            else{
               perror("Failed to receive ACK from destination server");
            }

            const char *success_msg = "File copy operation between different storage servers completed.";
            send(client_socket, success_msg, strlen(success_msg), 0);
            log_message(success_msg, server->ip_address, server->client_port);
         }
         else {
            const char *error = "Unknown command";
            send(client_socket, error, strlen(error), 0);
         }
         pthread_mutex_unlock(&naming_server.lock);
      }
   }   
   close(client_socket);
}

void* connection_handler(void* socket_desc) {
   int client_socket = *(int*)socket_desc;
   free(socket_desc);
   
   // First message determines if it's a storage server or client
   char buffer[BUFFER_SIZE];
   memset(buffer, 0, sizeof(buffer));
   ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
   
   if (bytes_received > 0) {
      buffer[bytes_received] = '\0';
      printf(MAGENTA"Received message: %s\n", buffer);

      struct sockaddr_in client_addr;
      socklen_t addr_len = sizeof(client_addr);
      getpeername(client_socket, (struct sockaddr *)&client_addr, &addr_len);
      const char *ip = inet_ntoa(client_addr.sin_addr);
      int port = ntohs(client_addr.sin_port);

      if (strcmp(buffer, "STORAGE_SERVER") == 0) {
         log_message("Storage Server connected: ", ip, port);
         handle_storage_server_registration(client_socket);
         printf("Storage Server registration..\n");
      } 
      else if (strcmp(buffer, "CLIENT") == 0) {
         log_message("Client connected: ", ip, port);
         handle_client_request(client_socket);
      }
      // else if (strcmp(buffer, "ASYNC") == 0) {  
      //    async_handler(client_socket);
      // }
   }
   else{
      perror(RED"recv failed"RESET);
   }
   printf(RESET);
   
   return NULL;
}

void async_handler(int server_socket) {
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
