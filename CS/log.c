#include "headers.h"

// int insert_in_log(const int type, const int ss_id, const int ss_or_client_port, const int request_type, const char* request_data, const int status_code)
int insert_in_log(int type, int ss_id, int ns_port, int ss_or_client_port, int request_type, char* path, int isAck){
    FILE* fptr = fopen("logs.txt" , "a");
    if (fptr == NULL){
        perror("Failed to open log file");
        return -1;
    }

    char* request_names[] = {"CREATE", "READ", "WRITE", "DELETE", "COPY", "STREAM"};

    if(type == SS){
        if(isAck){

        }
        else{
            // fprintf(fptr, "Storage Server %d connected on port %d\n", ss_id, ss_or_client_port);
            fprintf(fptr, "Storage Server %d connected on port %d\n", ss_id, ss_or_client_port);
        }

    }
    else if(type == CLIENT){
        if (isAck){

        }
        else{
            fprintf(fptr, "Client connected on port %d is making a %s request for path %s.\n", ss_or_client_port, request_names[request_type], path);

        }

    }    

    fclose(fptr);

    return 1;
}