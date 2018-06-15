#ifndef __PARSER_H__
#define __PARSER_H__
#include <stdio.h>
#include <stdlib.h>
#include "circular-buffer.h"
#include <stdbool.h>
#include "utils.h"
#include "structs.h"

void parser_parses(parser_t * parser, char * file_path);
bool put_data(parser_t *parser, unsigned char *data, int length);

void SUL_init(SUL_t * sul);
void init_parser(parser_t *parser);
void init_visible_record(visible_record_t *vr);
void init_logical_record(logical_record_t *lr);
void init_lr_segment(lr_segment_t *lr_segment);
void init_lr_attribute(lr_attribute_t* attr);
void init_lr_object(lr_object_t* obj, lr_attribute_t* attr);
void init_obname_t(obname_t* obname);
void init_fdata_t(fdata_t* fdata, int number_of_channel);
void init_frame_t(frame_t* frame, lr_object_t frame_obj, parser_t * parser);

bool parse_sul(parser_t * parser);
bool parse_visible_record(parser_t *parser);

void parse_logical_record(parser_t *parser, unsigned char * body, int length);
int lr_read_component_role(unsigned char c);
int lr_read_component_format(unsigned char c);
void lr_segment_read_attributes (lr_segment_t * segment, unsigned char c);
int lr_read_attribute(logical_record_t* current_lr, int format, unsigned char* data);
int lr_read_object(logical_record_t* current_lr, unsigned char* data);
void lr_read_absatr(logical_record_t* current_lr);
int lr_read_componet_values(lr_attribute_t* attr, unsigned char* data);
void lr_parse_iflr(parser_t* parser, unsigned char* data);

void print_visible_records(parser_t *parser);
void print_logical_record(parser_t *parser);
void print_datasets(parser_t *parser);
void print_channels(parser_t *parser); // show all curves 

        
#endif
