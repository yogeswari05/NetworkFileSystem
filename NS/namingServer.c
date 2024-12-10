#include "headers.h"
#include "helper.h"
#include "namingServer.h"
#include "cache2.h"

int main(int argc, char *argv[]) {

   init_cache();

   // Check for correct number of arguments
   if (argc != 2) {
      // printf("Usage: %s <port>\n", argv[0]);
      printf(RED "Usage: %s <port>\n" RESET, argv[0]);
      return 1;
   }

   char ip_address[16] = {0};
   int port = atoi(argv[1]);

   // Get the local IP address
   get_local_ip(ip_address);

   printf(YELLOW "\nNaming Server will use IP Address: %s and Port: %d\n" RESET, ip_address, port);

   int server_socket;
   struct sockaddr_in server_addr;

   // Initialize naming server
   pthread_mutex_init(&naming_server.lock, NULL);
   naming_server.num_storage_servers = 0;

   // Create socket
   server_socket = socket(AF_INET, SOCK_STREAM, 0);
   if (server_socket < 0) {
      perror("Socket creation failed");
      return 1;
   }

   // Configure server address
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = inet_addr(ip_address); // Use the retrieved local IP address
   server_addr.sin_port = htons(port);

   // Bind socket
   if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
      // perror("Bind failed");
      perror(RED"Bind failed"RESET);
      return 1;
   }

   // Listen for connections
   if (listen(server_socket, MAX_STORAGE_SERVERS + MAX_CLIENTS) < 0) {
      perror(RED"Listen failed"RESET);
      return 1;
   }

   printf(YELLOW "Naming Server started on %s:%d\n"RESET, ip_address, port);

   // Accept incoming connections
   while (1) {
      struct sockaddr_in client_addr;
      socklen_t addr_len = sizeof(client_addr);
      int *client_socket = malloc(sizeof(int));
      *client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);

      if (*client_socket < 0) {
         perror("Accept failed");
         free(client_socket);
         continue;
      }

      printf(BLUE"New connection from %s:%d\n"RESET,
            inet_ntoa(client_addr.sin_addr),
            ntohs(client_addr.sin_port));

      // create thread to listen for ack from storage server in case of async writing
      // pthread_t ack_thread;
      // if (pthread_create(&ack_thread, NULL, async_handler, (void *)client_socket) < 0) {
      //    perror("Thread creation failed");
      //    free(client_socket);
      //    continue;
      // } 
      // else {
      //    printf(BLUE"Thread created\n\n"RESET);
      // }

      // Create thread to handle connection
      pthread_t thread_id;
      if (pthread_create(&thread_id, NULL, connection_handler, (void *)client_socket) < 0) {
         perror("Thread creation failed");
         free(client_socket);
         continue;
      } 
      else {
         printf(BLUE"Thread created\n\n"RESET);
      }

      pthread_detach(thread_id);
   }

   close(server_socket);
   // pthread_join(ack_thread, NULL);
   pthread_mutex_destroy(&naming_server.lock);
   return 0;
}
