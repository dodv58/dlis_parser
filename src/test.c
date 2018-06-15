#include "utils.h"
#include "circular-buffer.h"
void test_cbuffer() {
    cbuffer_t cbuffer;
    cbuffer_init(&cbuffer);

    cbuffer_insert(&cbuffer, "abcde", 5);
    
    char out_buffer[11];
    cbuffer_get(&cbuffer, out_buffer, 11, 5);
    print_log(out_buffer);
    cbuffer_insert(&cbuffer, "fghikl", 6);
    cbuffer_get(&cbuffer, out_buffer, 11, 3);
    cbuffer_insert(&cbuffer, "mnop", 4);
    cbuffer_get(&cbuffer, out_buffer, 11, 10);
    print_log(out_buffer);
    int nbytes = cbuffer_get(&cbuffer, out_buffer, 11, 10);
    
    print_log("out_buffer: ");
    print_log(out_buffer);
    print_log("origin: ");
    print_log(cbuffer.buffer);
}

void test() { 
    char ** array;
    array = malloc(10 * sizeof(char *));
    for(int i = 0; i < 10; i++){
        array[i] = malloc(10);
        array[i] = "hello";
    }
    for(int i = 0; i < 10; i++){
        printf("%s\n", array[1]);
    }

    char result[50];
    float num = 23.34;
    sprintf(result, "%f", num);
    printf("\n The string for the num is %s", result);
}

typedef struct linked_list_s {
    void* node;
    struct linked_list_s * next;
} linked_list_t;

int main() {
    // test_cbuffer();
    test();
	return 0;
}

