#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

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
	char str[100] = "000.5";
    int res = is_number(str, strlen(str));
    if (res) {
        int val = strtol(str, NULL, 10);
        printf("val:%d\n",val);
    }
    else {
        printf("Not a number\n");
    }
	return 0;
}
