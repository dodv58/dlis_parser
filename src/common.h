#ifndef __DLIS_COMMON__
#define __DLIS_COMMON__
#include <arpa/inet.h>
typedef unsigned char byte_t;

struct sized_str_s {
    byte_t *buff;
    int len;
};
typedef struct sized_str_s sized_str_t;
typedef enum rep_code_e rep_code_t;

enum rep_code_e {
    DLIS_FSHORT = 	1,		//2
    DLIS_FSINGL = 	2,		//4
    DLIS_FSING1 = 	3,		//8
    DLIS_FSING2 = 	4,		//12
    DLIS_ISINGL = 	5,		//4
    DLIS_VSINGL = 	6,		//4
    DLIS_FDOUBL = 	7,		//8
    DLIS_FDOUB1 = 	8,		//16
    DLIS_FDOUB2 = 	9,		//24
    DLIS_CSINGL = 	10,		//8
    DLIS_CDOUBL = 	11,		//16
    DLIS_SSHORT = 	12,		//1
    DLIS_SNORM = 	13,		//2
    DLIS_SLONG = 	14,		//4
    DLIS_USHORT = 	15,		//1
    DLIS_UNORM = 	16,		//2
    DLIS_ULONG = 	17,		//4
    DLIS_UVARI = 	18,		//1, 2 or 4
    DLIS_IDENT = 	19,		//V
    DLIS_ASCII = 	20,		//V
    DLIS_DTIME = 	21,		//8
    DLIS_ORIGIN = 	22,		//V
    DLIS_OBNAME = 	23,		//V
    DLIS_OBJREF = 	24,		//V
    DLIS_ATTREF = 	25,		//V
    DLIS_STATUS = 	26,		//1
    DLIS_UNITS = 	27,		//V
    DLIS_REPCODE_MAX = 28
};


int parse_ushort(byte_t *data);
int parse_unorm(byte_t *data);
int parse_sshort(byte_t *data);
int parse_snorm(byte_t *data);
int parse_snorm(byte_t *data);
unsigned int parse_ulong(byte_t *data);
int parse_uvari(byte_t* buff, int buff_len, int* data); //return nbytes read
int parse_ident(byte_t *buff, int buff_len, sized_str_t *output);
//int parse_value(byte_t* buff, int buff_len, int rep_code, void* output);
int parse_values(byte_t *buff, int buff_len, int val_cnt, int rep_code);

size_t trim(char *out, size_t len, const char *str);
void hexDump (char *desc, void *addr, int len);
int is_integer(char *str, int len);
#endif
