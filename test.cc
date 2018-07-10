#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "src/common.h"
/*
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

void test_parse_fsingl() {
    float fval;
    float expected = 153.0;
    byte_t in_data[4] = {0x43,0x19,0x00,0x00 };
    fval = parse_fsingl(in_data);
    printf("--- TEST parse_fsingl ----\n");
    printf("parse_fsingl: %f, expected: %f\n", fval, expected);
}

void test_parse_fdoubl() {
    byte_t buff1[] = {
        0b01000000, 0b01100011, 0b00100000, 0b00000000, 0x00, 0x00, 0x00, 0x00
    };
    double expected1 = 153.;
    byte_t buff2[] = {
        0b11000000, 0b01100011, 0b00100000, 0b00000000, 0x00, 0x00, 0x00, 0x00
    };
    double expected2 = -153.;

    printf("--- TEST parse_fdoubl ----\n");
    double value = parse_fdoubl(buff1);
    printf("parse_fdoubl: %f -- expected:%f\n", value, expected1);
    value = parse_fdoubl(buff2);
    printf("parse_fdoubl: %f -- expected:%f\n", value, expected2);
}

void test_parse_ident() {
    byte_t buff[] = {
        0x05, 'h', 'e', 'l', 'l', 'o'
    };
    char *expected = "hello";
    int expected1 = 6;
    sized_str_t lstr;
    int len = parse_ident(buff, sizeof(buff), &lstr);
    printf("--- TEST parse_ident ----\n");
    printf("parse_ident: %.*s, %d -- expected: %s, %d\n", lstr.len, lstr.buff, len, expected, expected1);
}

void test_parse_uvari() {
    byte_t buff1[] = {
        0x42
    };
    byte_t buff2[] = {
        0b10110100, 0b11010001 // 0x34D1
    };
    byte_t buff3[] = {
        0b11011100, 0b10100010,  0b11011100, 0b10100010 
        //    1    C    A    2    D    C    A    2
    };
    uint32_t expected1 = 0x42;
    uint32_t expected2 = 0x34D1;
    uint32_t expected3 = 0x1CA2DCA2;

    uint32_t value;
    printf("--- TEST parse_uvari ----\n");
    parse_uvari(buff1, sizeof(buff1), &value);
    printf("parse_uvari: %d -- expected: %d\n", value, expected1);
    parse_uvari(buff2, sizeof(buff2), &value);
    printf("parse_uvari: %d -- expected: %d\n", value, expected2);
    parse_uvari(buff3, sizeof(buff3), &value);
    printf("parse_uvari: %d -- expected: %d\n", value, expected3);
}

void test_parse_ascii() {
    byte_t buff[] = {
        0b10000000, 0b00001011,
        'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'
    };
    char *expected = "hello world";
    sized_str_t value;
    int expected1 = 13;
    printf("--- TEST parse_ascii ----\n");
    int len = parse_ascii(buff, sizeof(buff), &value);
    printf("parse_ascii: %.*s, %d -- expected:%s, %d\n", value.len, value.buff, len, expected, expected1);
}

void test_parse_obname() {
    byte_t buff[] = {
        0x01, 0x01, 0x05, 'h', 'e', 'l', 'l', 'o'
    };
    obname_t expected = {
        .origin = 1,
        .copy_number = 1,
        .name = {
            .buff = "hello", 
            .len = 5
        }
    };
    obname_t obname;
    int len = parse_obname(buff, sizeof(buff), &obname);
    printf("--- TEST parse_obname ----\n");
    print_obname(&obname);
    print_obname(&expected);
}
*/

struct a {
    int val1;
    double val2;
};
void test_linhtinh() {
    struct a a_val, b_val;
    a_val.val1 = 10;
    a_val.val2 = -1.5;

    b_val = a_val;

    printf("%d, %f\n", b_val.val1, b_val.val2);
}

void test_parse_dtime() {
    byte_t buff[] = { 0b01010111, 0b00010100, 0b00010011, 0b00010101, 0b00010100, 0b00001111, 0b00000010, 0b01101100};
    dtime_t dtime;
    parse_dtime(buff, 8, &dtime);
    print_dtime(&dtime);
    printf("\n");
}
int main( int argc, char *argv[] ) {
    /*test_parse_fsingl();
    test_parse_fdoubl();
    test_parse_ident();
    test_parse_uvari();
    test_parse_ascii();
    test_parse_obname();*/
    test_linhtinh();
    test_parse_dtime();
	return 0;
}
