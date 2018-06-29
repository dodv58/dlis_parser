#ifndef __DLIS_COMMON__
#define __DLIS_COMMON__
size_t trim(char *out, size_t len, const char *str);
void hexDump (char *desc, void *addr, int len);
int is_integer(char *str, int len);
#endif
