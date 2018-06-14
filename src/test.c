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
    unsigned int x = 2415919104;
    printf("%d\n", x << 1);
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

