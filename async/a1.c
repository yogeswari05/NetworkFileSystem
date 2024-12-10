#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define CHUNK_SIZE 1024  // Fixed size chunks for flushing
#define LARGE_THRESHOLD 1048576  // Threshold for large requests (1MB)

void handle_write(int client_socket, char* path, char* data, size_t data_size, int is_sync) {

    // Check if the data size is large enough for asynchronous write
    if (data_size <= LARGE_THRESHOLD || is_sync) {
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
        pthread_detach(write_thread);
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
    }

    fclose(file);
    printf("Asynchronous write complete for file: %s\n", args->path);

    // Notify NS about success (simulated here with a print)
    printf("Notifying NS about completion\n");

    free(args->buffer);
    free(args->path);
    free(args);
    return NULL;
}

// NO LOCKS AND NO ERROR HANDLING IF SERVER FAILS AND NO PERIODIC WRITING 
