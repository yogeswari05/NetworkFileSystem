#include "errors.h"

// Function definition
const char* get_error_message(char * error_code) {
    if (strcmp(error_code, ERROR_FILE_NOT_ACCESSIBLE) == 0) {
        return "Error: The requested file is not an accessible path.";
    } else if (strcmp(error_code, ERROR_FILE_IN_USE_BY_WRITE) == 0) {
        return "Error: The requested file is currently being written to by another client.";
    } else {
        return "Unknown error.";
    } return "Unknown error.";
    }
}
