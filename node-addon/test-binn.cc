#include <stdlib.h>
#include <binn.h>
#include <stdio.h>
#include <string.h>

void get_repcode_dimension(int input, int *repcode, int *dimension) {
    *repcode = input >> 16;
    *dimension = input & 0x0000FFFF;
}

int pack(int repcode, int dimension) {
    return repcode << 16 | dimension;
}

int test() {
    int input = pack(21, 1984);
    int repcode;
    int dimension;
    get_repcode_dimension(input, &repcode, &dimension);
    printf("%d, %d\n", repcode, dimension);
    return 0;
}

int test_binn() {
    binn *obinn = binn_object();
    binn_object_set_int32(obinn, (char *)"id", 12);
    binn_object_set_str(obinn, (char *)"value", "UET VNU uni");

    char buffer[500];
    memcpy(buffer, binn_ptr(obinn), binn_size(obinn));
    
    int id = 0;
    id = binn_object_int32(buffer, (char *)"id");
    char * value = 0;
    value = binn_object_str(buffer, (char *)"value");
    printf("id=%d, value=%s, %d\n", id, value, sizeof(int));
    return 0;
}

int main() {
    return test();
}
