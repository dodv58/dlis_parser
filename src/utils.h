#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include "constants.h"
#include <string.h>
#include <stdlib.h>
#include "structs.h"
#include <inttypes.h>

#define print_log(msg) doPrint((char *)__func__,(char *) msg)
void doPrint(char *func_name, char *msg);
int utils_read_data(void * dest, void *data, int code);
unsigned int utils_get_length(unsigned int code);
void hex_to_number(void *dest, void* data, int len);
//return number of bytes are read
int uvari_to_int(unsigned int * dest, void * data);
bool str_to_number(void *dest, void * data, int len);
int utils_read_obname(obname_t * dest, unsigned char * data);
int utils_read_objref(unsigned char* type, obname_t* obname, unsigned char* data);


#endif