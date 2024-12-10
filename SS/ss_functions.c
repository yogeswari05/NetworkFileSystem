#include "headers.h"
#include "helper.h"
#include "storageServer.h"
#include "errors.h"

void rwlock_init(rwlock_t *rw) {
   rw->readers = 0;
   sem_init(&rw->lock, 0, 1);
   sem_init(&rw->writelock, 0, 1);
}

void rwlock_acquire_readlock(rwlock_t *rw) {
   sem_wait(&rw->lock);
   rw->readers++;
   if (rw->readers == 1)
      sem_wait(&rw->writelock); // first reader acquires writelock
   sem_post(&rw->lock);
}

void rwlock_release_readlock(rwlock_t *rw) {
   sem_wait(&rw->lock);
   rw->readers--;
   if (rw->readers == 0)
      sem_post(&rw->writelock); // last reader releases writelock
   sem_post(&rw->lock);
}

void rwlock_acquire_writelock(rwlock_t *rw) {
   sem_wait(&rw->writelock);
}

void rwlock_release_writelock(rwlock_t *rw) {
   sem_post(&rw->writelock);
}

// int rwlock_try_acquire_readlock(rwlock_t* rwlock) {
//     pthread_mutex_lock(&rwlock->mutex);
//     if (rwlock->writer_active || rwlock->write_wait_count > 0) {
//         // Writer is active, or writers are waiting, cannot acquire the lock
//         pthread_mutex_unlock(&rwlock->mutex);
//         return -1; // Indicate failure
//     }
//     rwlock->reader_count++;
//     pthread_mutex_unlock(&rwlock->mutex);
//     return 0; // Indicate success
// }

int rwlock_try_acquire_readlock(rwlock_t *rwlock) {
    // Try to acquire the main lock without blocking
    if (sem_trywait(&rwlock->lock) == 0) {
        // Increment the reader count
        rwlock->readers++;
        if (rwlock->readers == 1) {
            // First reader blocks the writelock
            if (sem_trywait(&rwlock->writelock) != 0) {
                // Failed to acquire writelock, revert reader count and release main lock
                rwlock->readers--;
                sem_post(&rwlock->lock);
                return -1; // Indicate failure
            }
        }
        // Release the main lock after modifying reader count
        sem_post(&rwlock->lock);
        return 0; // Successfully acquired read lock
    } else {
        // Failed to acquire the main lock
        return -1; // Indicate failure
    }
}



// Add this function to get all accessible paths
void get_accessible_paths(const char* base_path, char paths[][MAX_PATH_LENGTH], int* num_paths, int* base_path_added) {
   // Add the base directory itself to the list once
   if (*base_path_added == 0) {
      snprintf(paths[*num_paths], MAX_PATH_LENGTH, "%s", base_path);
      (*num_paths)++;
      *base_path_added = 1;  // Set the flag to prevent future additions of base path
   }

   DIR* dir = opendir(base_path);
   if (dir == NULL) {
      printf("Failed to open directory: %s\n", base_path);
      return;
   }

   struct dirent* entry;
   char full_path[MAX_PATH_LENGTH];

   while ((entry = readdir(dir)) != NULL) {
      // Skip . and .. directories
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
         continue;
      }

      // Construct full path for the file/directory
      snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", base_path, entry->d_name);

      if (entry->d_type == DT_REG) {
         // If it's a regular file, add to the list
         snprintf(paths[*num_paths], MAX_PATH_LENGTH, "%s", full_path);
         (*num_paths)++;
      }
      else if (entry->d_type == DT_DIR) {
         // If it's a directory, add it to the list once and recurse
         snprintf(paths[*num_paths], MAX_PATH_LENGTH, "%s", full_path);
         (*num_paths)++;
         // Recursively explore subdirectories
         get_accessible_paths(full_path, paths, num_paths, base_path_added);
      }
   }

   closedir(dir);
}

void handle_delete(const char* path) {
   if (remove(path) == 0) {
      printf("Deleted: %s\n", path);
   } else {
      perror("Delete failed");
   }
}

void handle_read(int client_socket, const char* path) {
   printf("Received file path from client: %s\n", path);

   // Open the requested file
   FILE *file = fopen(path, "rb");
   if (!file) {
      perror("Failed to open file");
      const char *error_msg = "Error: File not found or unable to open\n";
      send(client_socket, error_msg, strlen(error_msg) + 1, 0);
      close(client_socket);
      return;
   }
   else{
      printf("File opened successfully\n");
   }
   // Send the file contents to the client
   char buffer[BUFFER_SIZE];
   size_t bytes_read;

   while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
      if (send(client_socket, buffer, bytes_read, 0) < 0) {
         perror("Failed to send file content to client");
         fclose(file);
         close(client_socket);
         return;
      }
   }
   // Send an EOF marker or a message indicating the end of the file
   const char *end_msg = "END_OF_FILE";
   send(client_socket, end_msg, strlen(end_msg) + 1, 0);

   // Close the file and the client socket
   fclose(file);
   close(client_socket);
}

// void handle_write(int client_socket, char* path, char* data) {
//    FILE *file = fopen(path, "w");
//    if (!file) {
//       perror("Failed to open file for writing");
//       const char *error_msg = "Error: Unable to write to file";
//       send(client_socket, error_msg, strlen(error_msg) + 1, 0);
//       close(client_socket);
//       return;
//    }
//    else{
//       printf("File opened successfully for writing\n");
//    }
//    fwrite(data, 1, strlen(data), file);
//    fclose(file);
//    // Send success message
//    printf("File written successfully\n");
//    const char *success_msg = "File written successfully";
//    send(client_socket, success_msg, strlen(success_msg) + 1, 0);
//    close(client_socket);
// }

void handle_write(int client_socket, char* path, char* data, size_t data_size, int is_sync, int name_server_socket) {

   // Check if the data size is large enough for asynchronous write
   printf("Data size: %ld\n", data_size);
   printf("LARGE_THRESHOLD: %d\n", LARGE_THRESHOLD);

   char temp[MAX_DATA_SIZE];
   strcpy(temp, data);

   char* token = strtok(temp, " ");
   if(strcmp(token, "--SYNC") == 0){
      is_sync = 1;
      // token = strtok(data, " ");
   }

   if (data_size <= LARGE_THRESHOLD || is_sync) {
      printf("Synchronous write\n");

      if(is_sync){
         printf("Sync write\n");
         data_size -= 7;
         data = data + 7;
      }

      // Synchronous write
      FILE *file = fopen(path, "w");
      if (!file) {
         perror("Failed to open file for writing");
         const char *error_msg = "Error: Unable to write to file";
         send(client_socket, error_msg, strlen(error_msg) + 1, 0);
      } else {
         printf("File opened successfully for writing\n");
         fwrite(data, 1, strlen(data), file);
         fclose(file);
         // Send success message
         printf("File written successfully\n");
         const char *success_msg = "File written successfully";
         send(client_socket, success_msg, strlen(success_msg) + 1, 0);
      }
      close(client_socket);
   } else {
      // Asynchronous write
      printf("Buffering data for asynchronous write\n");
      
      // Simulate buffering
      char *buffer = malloc(data_size);
      if (!buffer) {
         const char *error_msg = "Error: Insufficient memory for buffering";
         send(client_socket, error_msg, strlen(error_msg) + 1, 0);
         close(client_socket);
         return;
      }
      // Copy data to buffer
      memcpy(buffer, data, data_size);

      // Send immediate acknowledgment
      const char *ack_msg = "Request accepted for asynchronous write";
      send(client_socket, ack_msg, strlen(ack_msg) + 1, 0);
      close(client_socket);

      // Dispatch a thread for actual writing
      pthread_t write_thread;
      struct write_args {
         char *path;
         char *buffer;
         size_t data_size;
      } *args = malloc(sizeof(struct write_args));
      args->path = strdup(path);
      args->buffer = buffer;
      args->data_size = data_size;
      // Create a detached thread for asynchronous write
      pthread_create(&write_thread, NULL, async_write, (void *)args);
      // Detach the thread
      // pthread_detach(write_thread);
      pthread_join(write_thread, NULL);
      // printf(RED"sleeping for 5 seconds\n");
      // sleep(5);
      // printf("Woke up\n"RESET);
   }
}

void* async_write(void *arg) {
   // Simulate asynchronous write
   struct write_args *args = (struct write_args *)arg;

   // Open the file for writing
   FILE *file = fopen(args->path, "w");
   if (!file) {
      perror("Failed to open file for asynchronous write");
      free(args->buffer);
      free(args->path);
      free(args);
      return NULL;
   }

   // Write in chunks
   size_t written = 0;
   while (written < args->data_size) {
      size_t chunk = (args->data_size - written > CHUNK_SIZE) ? CHUNK_SIZE : (args->data_size - written);
      fwrite(args->buffer + written, 1, chunk, file);
      written += chunk;

      // Simulate flush
      fflush(file);
      sleep(2);
   }

   fclose(file);
   printf("Asynchronous write complete for file: %s\n", args->path);

   // Notify NS about success (simulated here with a print)
   printf("Notifying NS about completion\n");
   // send ack to ns
   // char *ack_msg = "Asynchronous write completed for file\n";
   // sprintf(ack_msg, "ASYNC %s", ack_msg);
   // send(args->nm_socket, ack_msg, strlen(ack_msg) + 1, 0);
   // send(client_socket, ack_msg, strlen(ack_msg) + 1, 0);

   free(args->buffer);
   free(args->path);
   free(args);
   return NULL;
}

// NO LOCKS AND NO ERROR HANDLING IF SERVER FAILS AND NO PERIODIC WRITING 


void handle_get_file_info(int client_socket, char* path) {
   // Get file information
   struct stat file_stat;
   if (stat(path, &file_stat) != 0) {
      perror("Failed to get file info");
      const char *error_msg = "Error: Unable to retrieve file info";
      send(client_socket, error_msg, strlen(error_msg) + 1, 0);
      close(client_socket);
      return;
   }
   // Prepare and send file info
   char response[256];
   snprintf(response, sizeof(response), "Size: %ld bytes, Permissions: %o",
            file_stat.st_size, file_stat.st_mode & 0777);
   send(client_socket, response, strlen(response) + 1, 0);
   printf("File info sent to the client: %s\n", response);
   close(client_socket);
}

void handle_stream_audio(int client_socket, char* path) {

   char temp[MAX_PATH_LENGTH];
   strcpy(temp, path);

   char* token = strtok(temp, ".");
   token = strtok(NULL, ".");

   if(strcmp(token, "mp3") != 0){
      char response[BUFFER_SIZE];
      // sprintf(response, "", path);
      // const char *error_msg = ERROR_INCORRECT_AUDIO_FORMAT;
      sprintf(response, "%s", ERROR_INCORRECT_AUDIO_FORMAT);
      printf("sending audio error code to client\n");

      // send(client_socket, error_msg, strlen(error_msg) + 1, 0);

      if(send(client_socket, response, strlen(response), 0) < 0){
         perror("Failed to send server info to client");
      }
      else{
         printf("sent error code successfully\n");
      }
      close(client_socket);
      return;
   }

  FILE *file = fopen(path, "rb");
   if (!file) {
      perror("Failed to open audio file");
      const char *error_msg = "Error: Unable to open audio file";
      send(client_socket, error_msg, strlen(error_msg) + 1, 0);
      close(client_socket);
      return;
   }

   // Stream the file contents
   char buffer[BUFFER_SIZE];
   size_t bytes_read;

   while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
      if (send(client_socket, buffer, bytes_read, 0) < 0) {
         perror("Failed to send audio data");
         fclose(file);
         close(client_socket);
         return;
      }
   }

   fclose(file);
   close(client_socket);
}

void* handle_client(void* arg) {
   ClientHandler* handler = (ClientHandler*)arg;
   char buffer[BUFFER_SIZE];
   printf(CYAN"Client connected\n");
   while (1) {
      memset(buffer, 0, BUFFER_SIZE);
      // Receive client request
      ssize_t bytes_received = recv(handler->client_socket, buffer, BUFFER_SIZE - 1, 0);

      if (bytes_received <= 0) break;

      buffer[bytes_received] = '\0';
      printf("Received message from the client: %s\n", buffer);
      char command[32];
      char path[MAX_PATH_LENGTH];
      char data[BUFFER_SIZE]; // For storing the data part of "WRITE" command
      data[0] = '\0';         // Clear the data buffer
      
      // Parse the client request
      int args = sscanf(buffer, "%s %s %[^\n]", command, path, data); // %s %s %[^\n]

      if (strcmp(command, "READ") == 0){
         if(args < 2){
            printf("ERROR: READ command requires at least 2 arguments (READ <path>)\n");
            continue;
         }
         printf("Read request for: %s\n", path);
         // insert_in_log(CLIENT, 0, 0, handler->client_port, READ_REQUEST, path, 0);
         printf(YELLOW"ready to read\n"RESET);

         // rwlock_acquire_readlock(&rwlock);
         // printf(YELLOW"read lock acquired\n"RESET);
         // handle_read(handler->client_socket, path);
         // printf(YELLOW"read done\n"RESET);
         // rwlock_release_readlock(&rwlock);
         // printf(YELLOW"read lock released\n"RESET);

         if (rwlock_try_acquire_readlock(&rwlock) == 0) { // Successfully acquired the read lock
            printf(YELLOW "Read lock acquired\n" RESET);
         handle_read(handler->client_socket, path);
            printf(YELLOW "Read done\n" RESET);
         rwlock_release_readlock(&rwlock);
         } else {
            printf(RED "Read lock not available, exiting read operation\n" RESET);
            // Optionally, handle the failure case
         }

      }
      else if (strcmp(command, "WRITE") == 0){
         if(args < 3){
               printf("ERROR: WRITE command requires at least 3 arguments (WRITE <path> <data>)\n");
               continue;
         }
         printf("Write request for: %s\n", path);
         // last argument is the sync flag, to be added later
         printf("Size of data: %ld\n", strlen(data));
         printf(YELLOW"ready to write\n");
         rwlock_acquire_writelock(&rwlock);
         printf(YELLOW"write lock acquired\n");
         handle_write(handler->client_socket, path, data, strlen(data), 0, handler->nm_socket);
         printf(YELLOW"write done\n");
         rwlock_release_writelock(&rwlock);
         printf(YELLOW"write lock released\n"RESET);
      }
      else if (strcmp(command, "GET") == 0){
         if(args < 2){
            printf("ERROR: GET command requires at least 2 arguments (GET <path>)\n");
            continue;
         }
         printf("Get request for: %s\n", path);
         handle_get_file_info(handler->client_socket, path);
      }
      else if (strcmp(command, "STREAM") == 0){
         if(args < 2){
            printf("ERROR: STREAM command requires at least 2 arguments (STREAM <path>)\n");
            continue;
         }
         printf("Stream request for: %s\n", path);
         handle_stream_audio(handler->client_socket, path);
      }
   }
   // close(handler->client_socket);
   // free(handler);
   printf(RESET);
   return NULL;
}

int create_directories(const char *path) {
   char temp[MAX_PATH_LENGTH];
   char *pos = NULL;
   size_t len;

   // Copy the path into temp to work with it
   snprintf(temp, sizeof(temp), "%s", path);

   // Remove trailing slash if any
   if (temp[strlen(temp) - 1] == '/') {
      temp[strlen(temp) - 1] = '\0';
   }
   // Iterate through the path and create directories as needed
   for (pos = temp + 1; (pos = strchr(pos, '/')) != NULL; ++pos) {
      *pos = '\0';
      // Try creating the directory
      if (mkdir(temp, S_IRWXU) == -1 && errno != EEXIST) {
         return -1; // Error in creating directory
      }
      *pos = '/'; // Restore the slash for the next iteration
   }
   // Finally, create the last directory/file (if necessary)
   if (mkdir(temp, S_IRWXU) == -1 && errno != EEXIST) {
      return -1; // Error in creating directory
   }
   printf("Directory structure created for: %s\n", path);
   return 0; // Success
}

// Function to check if a path is a directory
int is_directory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
      return 1;  // It's a directory
   }
   return 0;  // It's not a directory
}

// Function to copy a file
int copy_file(const char *source, const char *destination) {
   FILE *src = fopen(source, "r");
   FILE *dest = fopen(destination, "w");

   if (!src || !dest) {
      if (src) fclose(src);
      if (dest) fclose(dest);
      return -1;  // Failure to open file
   }
   char file_buffer[BUFFER_SIZE];
   size_t bytes;
   while ((bytes = fread(file_buffer, 1, BUFFER_SIZE, src)) > 0) {
      fwrite(file_buffer, 1, bytes, dest);
   }
   fclose(src);
   fclose(dest);
   return 0;  // Success
}

// Function to recursively copy a directory
int copy_directory(const char *source_dir, const char *dest_dir) {
   DIR *dir = opendir(source_dir);
   if (!dir) {
      return -1;  // Failed to open source directory
   }
   // Create destination directory
   if (mkdir(dest_dir, 0755) != 0) {
      closedir(dir);
      return -1;  // Failed to create destination directory
   }
   struct dirent *entry;
   while ((entry = readdir(dir)) != NULL) {
      // Skip "." and ".."
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
         continue;
      }
      char source_path[BUFFER_SIZE];
      char dest_path[BUFFER_SIZE];
      snprintf(source_path, sizeof(source_path), "%s/%s", source_dir, entry->d_name);
      snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

      if (is_directory(source_path)) {
         // Recursively copy subdirectories
         if (copy_directory(source_path, dest_path) != 0) {
            closedir(dir);
            return -1;  // Failure to copy subdirectory
         }
      } 
      else {
         // Copy individual files
         if (copy_file(source_path, dest_path) != 0) {
            closedir(dir);
            return -1;  // Failure to copy file
         }
      }
   }
   closedir(dir);
   return 0;  // Success
}

// Function to check if a given path is a directory
// int is_directory__for_send(const char *path) {
//    struct stat statbuf;
//    if (stat(path, &statbuf) != 0) {
//       return 0; // Error checking file
//    }
//    return S_ISDIR(statbuf.st_mode);
// }


// Function to send a file or directory contents to the Naming Server
void send_file_or_directory(const char *path, int nm_socket) {
   char buffer[BUFFER_SIZE];
   FILE *file;
   struct dirent *entry;
   DIR *dp;
   // Check if the source path is a directory or file
   if (is_directory(path)) {
   // It's a directory: Send a marker indicating "DIRECTORY"
      memset(buffer, 0, sizeof(buffer));
      snprintf(buffer, sizeof(buffer), "DIRECTORY %s\n", path);
      send(nm_socket, buffer, strlen(buffer), 0);
      printf("Sent directory marker: %s\n", path);

      // Open the directory and send its contents recursively
      dp = opendir(path);
      if (dp == NULL) {
         perror("Failed to open directory");
         return;
      }
      while ((entry = readdir(dp)) != NULL) {
         // Skip . and .. entries
         if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
         }
         // Create the full path for the file/folder
         char full_path[BUFFER_SIZE];
         snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

         // Recursively send the files/folders
         send_file_or_directory(full_path, nm_socket);
      }
      closedir(dp);

      // Send an "END_DIRECTORY" marker after all contents of the directory
      const char *end_marker = "END_DIRECTORY\n";
      send(nm_socket, end_marker, strlen(end_marker), 0);
      printf("Sent END_DIRECTORY marker for: %s\n", path);
   } 
   else {
      // It's a file: Send a marker indicating "FILE"
      memset(buffer, 0, sizeof(buffer));
      snprintf(buffer, sizeof(buffer), "FILE %s\n", path);
      send(nm_socket, buffer, strlen(buffer), 0);
      printf("Sent file marker: %s\n", path);

      // Open the file and send its contents
      file = fopen(path, "rb");
      if (file == NULL) {
         perror("Failed to open file");
         return;
      }
      size_t bytes_read;
      while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
         send(nm_socket, buffer, bytes_read, 0);
      }
      fclose(file);

      // Send an "END_FILE" marker after the file is sent
      const char *end_marker = "END_FILE\n";
      send(nm_socket, end_marker, strlen(end_marker), 0);
      printf("Sent END_FILE marker for: %s\n", path);
   }
}

// Function to create a directory
int create_directory(const char *path) {
   if (mkdir(path, 0777) == -1) {
      perror("Failed to create directory");
      return -1;
   }
   return 0;
}

void receive_file_or_directory(int nm_socket, char* basepath_dest) {
   char buffer[BUFFER_SIZE];
   char current_directory[MAX_PATH_LENGTH] = {0};
   char full_path[MAX_PATH_LENGTH] = {0};
   FILE *file = NULL;
   int receiving_file = 0;

   while (1) {
      memset(buffer, 0, sizeof(buffer));
      ssize_t bytes_received = recv(nm_socket, buffer, sizeof(buffer) - 1, 0);
      if (bytes_received <= 0) {
         perror("Connection lost or error occurred");
         break;
      }
      // Check for the end of data marker
      if(strstr(buffer, "END_OF_DATA") != NULL){
         break;
      }

      buffer[bytes_received] = '\0'; // Null-terminate the received data
      printf("Received...: %s\n", buffer);

      char *current_position = buffer;
      while (*current_position != '\0') {
         // Check for control commands
         if (strncmp(current_position, "DIRECTORY ", 10) == 0) {
            if (receiving_file) {
               fclose(file);
               receiving_file = 0;
            }
            char directory_path[MAX_PATH_LENGTH];
            sscanf(current_position + 10, "%s", directory_path);
            const char *cleaned_path = (directory_path[0] == '.' && directory_path[1] == '/') ? directory_path + 2 : directory_path;

            snprintf(current_directory, sizeof(current_directory), "%s/%s", basepath_dest, cleaned_path);
            if (mkdir(current_directory, 0777) < 0 && errno != EEXIST) {
               perror("Failed to create directory");
            } 
            else {
               printf("Created directory: %s\n", current_directory);
            }
            current_position += strlen("DIRECTORY ") + strlen(directory_path) + 1;
         } 
         else if (strncmp(current_position, "END_DIRECTORY", 13) == 0) { 
            if (receiving_file) {
               fclose(file);
               receiving_file = 0;
            }
            printf("Finished processing directory: %s\n", current_directory);
            current_directory[0] = '\0';
            current_position += 13;
         }
         else if (strncmp(current_position, "FILE ", 5) == 0) {
            if (receiving_file) {
               fclose(file);
               receiving_file = 0;
            }
            char file_path[MAX_PATH_LENGTH];
            sscanf(current_position + 5, "%s", file_path);

            snprintf(full_path, sizeof(full_path), "%s/%s", basepath_dest, file_path);
            printf("Receiving file: %s\n", full_path);

            char *last_slash = strrchr(full_path, '/');
            if (last_slash) {
               *last_slash = '\0';
               if (mkdir(full_path, 0777) < 0 && errno != EEXIST) {
                  perror("Failed to create intermediate directories");
               }
               *last_slash = '/';
            }

            file = fopen(full_path, "wb");
            if (file == NULL) {
               perror("Failed to create file");
               continue;
            }
            else{
               printf("File:%s created successfully\n", full_path);
            }
            receiving_file = 1;
            current_position += strlen("FILE ") + strlen(file_path) + 1;
         }
         else if (strncmp(current_position, "END_FILE", 8) == 0) {
            if (receiving_file) {
               fclose(file);
               file = NULL;
               receiving_file = 0;
               printf("End of file received: %s\n", full_path);
            }
            current_position += 8;
            // Continue parsing the remaining buffer content
            while (*current_position == '\n' || *current_position == ' '){
               current_position++; // Skip any trailing newline or space characters
            }
         } 
         else {
            // Write remaining data to the file if we are currently receiving a file
            if (receiving_file && file != NULL) {
               char *end_marker = strstr(current_position, "END_FILE");
               if (end_marker) {
                  fwrite(current_position, 1, end_marker - current_position, file);
                  current_position = end_marker;
               } 
               else {
                  fwrite(current_position, 1, strlen(current_position), file);
                  break; // Exit loop to wait for more data
               }
            } 
            else {
               // If we aren't in a file block, just skip over non-command data
               current_position += strlen(current_position);
            }
         }
         // Ensure parsing continues for the remaining buffer content
         while (*current_position == '\n' || *current_position == ' '){
            current_position++; // Skip over any newlines or spaces before the next command
         }
      }
   }
}


void* handle_naming_server(void* arg) {
   NamingServerHandler* handler = (NamingServerHandler*)arg;
   char buffer[BUFFER_SIZE];

   while (1) {
      memset(buffer, 0, BUFFER_SIZE);
      ssize_t bytes_received = recv(handler->nm_socket, buffer, BUFFER_SIZE - 1, 0);

      if (bytes_received <= 0) {
         perror("Lost connection to naming server");
         break;
      }

      buffer[bytes_received] = '\0';
      printf("Message from Naming Server: %s\n", buffer);

      // Parse the command and paths
      char command[32];
      char flag[4] = {0};
      char path1[MAX_PATH_LENGTH] = {0};
      char path2[MAX_PATH_LENGTH] = {0};

      if (sscanf(buffer, "%s %s %s %s", command, path1, path2, flag) == 4){
         printf("Command: %s, Flag: %s, Path1: %s, Path2: %s\n", command, flag, path1, path2);
      }
      // Perform the requested operation
      if (strcmp(command, "CREATE") == 0) {
         char full_path[MAX_PATH_LENGTH];
         snprintf(full_path, sizeof(full_path), "%s/%s", path1, path2); // Combine path1 and path2
         
         // Ensure the directory exists
         printf("Performing CREATE operation with flag %s for: %s\n", flag, full_path);
         if (create_directories(path1) != 0) {
            snprintf(buffer, sizeof(buffer), "CREATE FAILED: Could not create directory structure for %s", full_path);
            printf("Failed to create directory structure\n");
         } 
         else {
            if (strcmp(flag, "-f") == 0) {
               // Create a file
               FILE *file = fopen(full_path, "w");
               if (file) {
                  fclose(file);
                  snprintf(buffer, BUFFER_SIZE, "CREATE FILE SUCCESS %s", full_path);
                  printf("File created successfully\n");
               } 
               else {
                  snprintf(buffer, BUFFER_SIZE, "CREATE FILE FAILED %s", full_path);
                  printf("Failed to create file\n");
               }
            } 
            else if (strcmp(flag, "-d") == 0) {
               // Create a directory
               if (mkdir(full_path, S_IRWXU) == 0 || errno == EEXIST) {
                  snprintf(buffer, BUFFER_SIZE, "CREATE DIRECTORY SUCCESS %s", full_path);
                  printf("Directory created successfully\n");
               } 
               else {
                  snprintf(buffer, BUFFER_SIZE, "CREATE DIRECTORY FAILED %s", full_path);
                  printf("Failed to create directory\n");
               }
            } 
            else {
               snprintf(buffer, BUFFER_SIZE, "CREATE FAILED: Invalid flag %s", flag);
               printf("Invalid flag\n");
            }
         }
         // Send acknowledgment back to the naming server
         send(handler->nm_socket, buffer, strlen(buffer), 0);
         printf("Acknowledgment sent to naming server: %s\n", buffer);
      } 
      else if (strcmp(command, "DELETE") == 0) {
         char full_path[MAX_PATH_LENGTH];
         snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", path1, path2); // Combine path1 and path2

         // Ensure the directory exists
         printf("Performing DELETE operation for: %s\n", full_path);
         // Simulate deletion of a file
         if (remove(full_path) == 0) {
            snprintf(buffer, BUFFER_SIZE, "DELETE SUCCESS %s", full_path);
            printf("File deleted successfully\n");
            send(handler->nm_socket, buffer, strlen(buffer), 0);
            printf("Acknowledgment sent to naming server: %s\n", buffer);
         } 
         else {
            snprintf(buffer, BUFFER_SIZE, "DELETE FAILED %s", full_path);
            printf("Failed to delete file\n");
         }
      } 
      else if (strcmp(command, "COPY") == 0) {
         printf("Performing COPY operation from %s to %s\n", path1, path2);
         struct stat statbuf;
         if (stat(path1, &statbuf) != 0) {
            snprintf(buffer, BUFFER_SIZE, "COPY FAILED %s to %s: Source does not exist", path1, path2);
            printf("Failed to find source path\n");
            send(handler->nm_socket, buffer, strlen(buffer), 0);
            continue;
         }

         if (S_ISDIR(statbuf.st_mode)) {
            printf("Source is a directory\n");
            // If source is a directory, handle recursively
            if (copy_directory(path1, path2) == 0) {
               snprintf(buffer, BUFFER_SIZE, "COPY SUCCESS directory %s to %s", path1, path2);
               printf("Directory copied successfully\n");
            } 
            else {
               snprintf(buffer, BUFFER_SIZE, "COPY FAILED directory %s to %s", path1, path2);
               printf("Failed to copy directory\n");
            }
         } 
         else {
            printf("Source is a file\n");
            // If source is a file, copy the file
            if (copy_file(path1, path2) == 0) {
               snprintf(buffer, BUFFER_SIZE, "COPY SUCCESS file %s to %s", path1, path2);
               printf("File copied successfully\n");
            } 
            else {
               snprintf(buffer, BUFFER_SIZE, "COPY FAILED file %s to %s", path1, path2);
               printf("Failed to copy file\n");
            }
         }
         // Send acknowledgment back to the naming server
         send(handler->nm_socket, buffer, strlen(buffer), 0);
         printf("Acknowledgment sent to naming server: %s\n", buffer);
      }
      else if(strcmp(command, "SEND") == 0){ // ns request, asking ss to send data of the path.
         printf("SEND command received for path: %s\n", path1);
         // Send the file or directory content to the Naming Server
         send_file_or_directory(path1, handler->nm_socket);
         char* end_marker = "END_OF_DATA";
         sleep(1);
         // send(handler->nm_socket, "END_OF_DATA", strlen("END_OF_DATA"), 0);
         if(send(handler->nm_socket, end_marker, strlen(end_marker)+1, 0) < 0){
            perror(RED"Failed to send end marker"RESET);
         }
         else{
            printf("End marker sent to Naming Server.\n");
         }
         printf("Data sent successfully to Naming Server.\n");
      }
      // else if (strncmp(buffer, "RECEIVE", 7) == 0){
      else if (strcmp(command, "RECEIVE") == 0){
         // Request to receive file or directory data from the Naming Server
         printf("RECEIVE command received for path: %s\n", path1);
         // Start receiving file or directory content from the Naming Server
         receive_file_or_directory(handler->nm_socket, path1);
         printf("Data received successfully from Naming Server.\n");

         const char *ack_message = "COPY operation is successful.";
         if (send(handler->nm_socket, ack_message, strlen(ack_message), 0) < 0) {
            perror(RED"Failed to send acknowledgment"RESET);
         } 
         else {
            printf(GREEN"Acknowledgment sent to Naming Server.\n"RESET);
         }

      }
      else {
         printf(RED"Unknown command!!\n"RESET);
         snprintf(buffer, BUFFER_SIZE, "UNKNOWN COMMAND");
         // send(handler->nm_socket, buffer, strlen(buffer), 0);
      }
   }
   close(handler->nm_socket);
   free(handler);
   return NULL;
}
