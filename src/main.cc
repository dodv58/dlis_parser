#include <stdlib.h>
#include <stdio.h>
#include "dlis.h"

int main(int argc, char *argv[]) {
    byte_t buffer[4 * 1024];
    char *file_name;
    int byte_read;

    file_name = (char *)"sample.dlis";
    if (argc >= 2) {
        file_name = argv[1];
    }
    dlis_t dlis;
    dlis_init(&dlis);
    FILE *f = fopen(file_name, "rb");
    if (f == NULL) {
        fprintf(stderr, "Error open file");
        exit(-1);
    }
    while(!feof(f) ) {
        byte_read = fread(buffer, 1, 4096, f);
        if (byte_read < 0) {
            fprintf(stderr,"Error reading file");
            exit(-1);
        }
        dlis_read(&dlis, buffer, byte_read);
    }
    printf("Finish reading file\n");
    return 0;
}
