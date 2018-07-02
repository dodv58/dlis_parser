#ifndef __DLIS_COMMON__
#define __DLIS_COMMON__

typedef unsigned char byte_t;
typedef sized_str_s sized_str_t;

struct sized_str_s {
    char *buff;
    int len;
};

int parse_ushort(byte_t *data);
int parse_unorm(byte_t *data);
int parse_uvari(int * output, int buff_len, byte_t *data); //return nbytes read
int parse_ident(char *buff, int buff_len, sized_str_t *output);
int parse_ulong(byte_t *data);

size_t trim(char *out, size_t len, const char *str);
void hexDump (char *desc, void *addr, int len);
int is_integer(char *str, int len);
#endif
