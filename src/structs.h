#ifndef __STRUCTS_H__
#define __STRUCTS_H__

#include <stdbool.h>
#include "circular-buffer.h"

typedef struct lr_attribute_s {
    unsigned char * label;
    unsigned int count;
    unsigned int representation_code;
    unsigned char * unit;
    unsigned char * value;
    struct lr_attribute_s * next;
} lr_attribute_t;

typedef struct lr_object_s {
    unsigned char * name;
    bool origin;
    int copy_number;
    lr_attribute_t attributes;
    lr_attribute_t * current_attr;
    struct lr_object_s * next;
} lr_object_t;

typedef struct logical_record_s {
	// TODO
    unsigned int code; //EFLR code < 256; IFLR code >= 256, IFLR type = code - 256.
    unsigned char * set_type; // type of set in EFLR
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


typedef struct fdata_s
{
    unsigned int index;
    unsigned char ** data;
    struct fdata_s * next;
} fdata_t;


typedef struct frame_s {
    lr_object_t frame;
    lr_object_t channels;
    lr_object_t* current_channel;
    unsigned int number_of_channel;
    unsigned int * representation_codes;
    fdata_t fdata;
    fdata_t* current_fdata;
    struct  frame_s * next;
} frame_t;

typedef struct parser_s {
    SUL_t sul;
	cbuffer_t cbuffer;
    visible_record_t root_vr;
    visible_record_t * current_vr;
    logical_record_t root_lr;
    logical_record_t * current_lr;
    frame_t frames;
    frame_t * current_frame;
    bool is_SUL_parsed;
} parser_t;

typedef struct obname_s {
    unsigned char * name;
    unsigned int origin;
    unsigned int copy_number;
} obname_t;

#endif