#ifndef __PARSER_H__
#define __PARSER_H__
#include <stdio.h>
#include <stdlib.h>
#include "circular-buffer.h"
#include <stdbool.h>
#include <inttypes.h>
#include "utils.h"

typedef struct lr_attribute_s {
    unsigned char * label;
    unsigned int count;
    unsigned int representation_code;
    unsigned char * unit;
    unsigned char * value;
    struct lr_attribute_s * next;
} lr_attribute_t;

typedef struct lr_object_s
{
    unsigned char * name;
    bool origin;
    int copy_number;
    lr_attribute_t attributes;
    lr_attribute_t * current_attr;
    struct lr_object_s * next;
} lr_object_t;

typedef struct logical_record_s {
	// TODO
    unsigned char * type;
    unsigned char * name;
    lr_attribute_t template;
    lr_attribute_t * current_attribute;

    lr_object_t objects;
    lr_object_t* current_obj;

    bool is_template_completed;
    struct logical_record_s * next;
} logical_record_t;

typedef struct SUL_s {
    unsigned int sequence_number;
    unsigned char version[6];
    unsigned char structure[7];
    unsigned long long max_length;
    unsigned char id[61];
} SUL_t;

typedef struct visible_record_s {
    unsigned int length;//2 bytes
    unsigned int version;//1 bytes
    unsigned int c_idx;
    struct visible_record_s* next;
} visible_record_t;

typedef struct logical_record_segment_s {
    unsigned int length;
    unsigned int type;
    bool structure;
    bool predecessor;
    bool successor;
    bool encryption;
    bool encryption_packet;
    bool checksum;
    bool trailing_length;
    bool padding;
} lr_segment_t;

typedef struct parser_s {
    SUL_t sul;
	cbuffer_t cbuffer;
    visible_record_t root_vr;
    visible_record_t * current_vr;
    logical_record_t root_lr;
    logical_record_t * current_lr;
} parser_t;

typedef struct obname_s {
    unsigned char * name;
    unsigned int origin;
    unsigned int copy_number;
} obname_t;


void parser_parses(parser_t * parser, char * file_path);
bool put_data(parser_t *parser, unsigned char *data, int length);

void init_SUL(SUL_t * sul);
void init_parser(parser_t *parser);
void init_visible_record(visible_record_t *vr);
void init_logical_record(logical_record_t *lr);
void init_lr_segment(lr_segment_t *lr_segment);
void init_lr_attribute(lr_attribute_t* attr);
void init_lr_object(lr_object_t* obj, lr_attribute_t* attr);
void init_obname_t(obname_t* obname);

bool parse_sul(parser_t * parser);
bool parse_visible_record(parser_t *parser);
logical_record_t *getLR(parser_t *parser);

void parse_LRS();
void parse_logical_record(parser_t *parser, unsigned char * body, int length);
int lr_read_component_role(unsigned char c);
int lr_read_component_format(unsigned char c);
void lr_segment_read_attributes (lr_segment_t * segment, unsigned char c);
int lr_read_attribute(logical_record_t* current_lr, int format, unsigned char* data);
int lr_read_object(logical_record_t* current_lr, unsigned char* data);

void print_visible_records(parser_t *parser);
void print_logical_record(parser_t *parser);
bool str_to_number(void *dest, void * data, int len);

int utils_read_obname(obname_t * dest, unsigned char * data);

extern bool is_SUL_parsed;    
#endif
