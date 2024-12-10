#include "headers.h"
#include "helper.h"
#include "storageServer.h"

rwlock_t rwlock;

int main(int argc, char *argv[]) {

   // Check for correct number of arguments
   if (argc < 6){
      printf(RED"\nUsage: %s <naming_server_ip> <naming_server_port> <ss_port> <port_for_clients> <base_path>\n"RESET, argv[0]);
      return 1;
   }


   // initialize the locks
   rwlock_init(&rwlock);

   // Get the command line arguments
   char *nm_ip = argv[1];           // Naming server IP
   int nm_port = atoi(argv[2]);     // Naming server port
   int sn_server_port = atoi(argv[3]); // Storage server port
   int client_port = atoi(argv[4]); // Client communication port

   // Get local IP address
   char server_ip[16] = {0};
   get_local_ip(server_ip);

   // Connect to Naming Server
   int nm_socket = socket(AF_INET, SOCK_STREAM, 0);
   if (nm_socket < 0) {
      perror("Socket creation failed");
      return 1;
   }
   
   // Bind to the specific source IP and port
   struct sockaddr_in source_addr;
   memset(&source_addr, 0, sizeof(source_addr));
   source_addr.sin_family = AF_INET;
   source_addr.sin_addr.s_addr = inet_addr(server_ip); // Use the server's IP
   source_addr.sin_port = htons(sn_server_port);       // Use the specified port

   if (bind(nm_socket, (struct sockaddr *)&source_addr, sizeof(source_addr)) < 0) {
      perror("Bind to source IP and port failed");
      return 1;
   }

   // Configure Naming Server address
   struct sockaddr_in nm_addr;
   memset(&nm_addr, 0, sizeof(nm_addr));
   nm_addr.sin_family = AF_INET;
   nm_addr.sin_addr.s_addr = inet_addr(nm_ip);
   nm_addr.sin_port = htons(nm_port);

   if (connect(nm_socket, (struct sockaddr *)&nm_addr, sizeof(nm_addr)) < 0) {
      perror("Connection to Naming Server failed");
      return 1;
   }

   // Send registration type
   const char *reg_type = "STORAGE_SERVER";
   if (send(nm_socket, reg_type, strlen(reg_type), 0) < 0) {
      perror("Initial registration send failed");
   } 
   else {
      printf(PINK"\nRegistration type sent to Naming Server\n");
   }
   sleep(1);

   // Create and send registration message
   char reg_msg[BUFFER_SIZE];
   char paths_str[BUFFER_SIZE] = "";

   // Create an array to store accessible paths
   char accessible_paths[100][MAX_PATH_LENGTH];
   int num_paths = 0;

   // Loop through all the remaining arguments to process base_path and directories/files
   printf("Count of files/directories: %d\n", argc - 5);
   for (int i = 5; i < argc; i++) {
      char *path = argv[i];
      printf("Processing path: %s\n", path);
      // Try to open the directory
      DIR *dir = opendir(path);
      if (dir != NULL) {
         // It's a directory, so get all files in the directory
         int base_path_added = 0;
         get_accessible_paths(path, accessible_paths, &num_paths, &base_path_added);
         closedir(dir);
      }
      else {
         // It's a file, so just add the file path to the accessible paths array
         snprintf(accessible_paths[num_paths], MAX_PATH_LENGTH, "%s", path);
         num_paths++;
      }
   }
   printf(RESET);

   // Print out all accessible paths
   printf(YELLOW"\nAll accessible paths:\n\n");
   for (int i = 0; i < num_paths; i++) {
      printf("%s\n", accessible_paths[i]);
   }
   printf("\n");
   printf(RESET);

   // Start with the number of paths as the first part of the message
   sprintf(reg_msg, "%s %d %d %d %d", server_ip, nm_port, sn_server_port, client_port, num_paths);

   // Append each path to the message
   for (int i = 0; i < num_paths; i++) {
      strcat(reg_msg, " ");
      strcat(reg_msg, accessible_paths[i]);
   }
   
   ssize_t bytesSent = send(nm_socket, reg_msg, strlen(reg_msg), 0);
   if (bytesSent < 0) {
      perror("Registration send failed");
   } 
   else {
      printf(GREEN"Storage Server registered. \nMessage: %s\n", reg_msg);
   }

   // Start Naming Server handler thread
   NamingServerHandler *nm_handler = malloc(sizeof(NamingServerHandler));
   nm_handler->nm_socket = nm_socket;

   pthread_t nm_thread_id;
   pthread_create(&nm_thread_id, NULL, handle_naming_server, nm_handler);

   // Start Client Server
   int server_socket = socket(AF_INET, SOCK_STREAM, 0);
   if (server_socket < 0) {
      perror(RED"Socket creation failed"RESET);
      return 1;
   }

   struct sockaddr_in server_addr;
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   char ip_address[16] = {0};
   get_local_ip(ip_address);
   server_addr.sin_addr.s_addr = inet_addr(ip_address);
   server_addr.sin_port = htons(client_port);

   if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
      perror("Bind failed");
      return 1;
   }

   if (listen(server_socket, MAX_CLIENTS) < 0) {
      perror("Listen failed");
      return 1;
   }

   printf(MAGENTA"Storage Server started. Listening for clients on port %d\n", client_port);
   printf(RESET);

   while (1) {
      struct sockaddr_in client_addr;
      socklen_t addr_len = sizeof(client_addr);
      // Accept client connection
      int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);

      if (client_socket < 0) {
         perror("Accept failed");
         continue;
      }
      else {
         printf("New connection from %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
      }

      ClientHandler *handler = malloc(sizeof(ClientHandler));
      handler->client_socket = client_socket;
      handler->nm_socket = nm_socket;

      pthread_t thread_id;
      pthread_create(&thread_id, NULL, handle_client, handler);
      pthread_detach(thread_id);
   }

   close(server_socket);
   close(nm_socket);
   return 0;
}
