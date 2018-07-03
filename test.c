#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "src/common.h"
int is_number(char *str, int len) {
    while (len > 0 && *str != 0 )  {
        if(!isdigit(*str)) {
            return 0;
        }
        str++;
        len--;
    }
    return 1;
}
int main( int argc, char *argv[] ) {
    /*
	char str[100] = "000.5";
    int res = is_number(str, strlen(str));
    if (res) {
        int val = strtol(str, NULL, 10);
        printf("val:%d\n",val);
    }
    else {
        printf("Not a number\n");
    }
    */
    byte_t in_data[4] = {0x00,0x01,0x02, 0b10011001 };
    int val = parse_ulong(in_data);
    printf("result : %d\n", val);

    int8_t neg_int = 1;
    hexDump("1", &neg_int, sizeof(int8_t));
    neg_int = -1;
    hexDump("-1", &neg_int, sizeof(int8_t));
    neg_int = 127;
    hexDump("127", &neg_int, sizeof(int8_t));
    neg_int = -127;
    hexDump("-127", &neg_int, sizeof(int8_t));

    byte_t tmp[2] = {0b11111111, 0b01100111};
    int sval = parse_snorm(tmp);
    printf("sval: %d", sval);
    
	return 0;
}
