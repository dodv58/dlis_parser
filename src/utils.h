#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include "constants.h"
#include <string.h>
#include <stdlib.h>

#define print_log(msg) doPrint((char *)__func__,(char *) msg)
void doPrint(char *func_name, char *msg);
int utils_read_data(void * dest, void *data, int code);
unsigned int utils_get_length(unsigned int code);
void hex_to_number(void *dest, void* data, int len);
//return number of bytes are read
int uvari_to_int(unsigned int * dest, void * data);


#endif