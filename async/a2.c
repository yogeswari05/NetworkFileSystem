#include <stdio.h>

int main() {
    printf("Loading... ");
    // fflush(stdout); // Forces "Loading... " to appear immediately

    // Simulate a delay
    for (int i = 0; i < 2000000000; i++);

    printf("Done!\n");
    return 0;
}
