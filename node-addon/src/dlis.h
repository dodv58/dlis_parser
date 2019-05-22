#ifndef __DLIS_H__
#define __DLIS_H__

#include "common.h"
#include <zmq.h>
#include <pthread.h>


#define ENDPOINT "ipc:///tmp/dlis-socket-"

enum lrs_iflr_type_e {
    FDATA = 0,
    NOFORMAT = 1,
    IFLR_RESERVED = 2, // for marking
    EOD = 127
};

enum lrs_eflr_type_e {
    FHLR = 0,
    OLR = 1,
    AXIS = 2,
    CHANNL = 3,
    FRAME = 4,
    STATIC = 5,
    SCRIPT = 6,
    UPDATE = 7,
    UDI = 8,
    LNAME = 9,
    SPEC = 10,
    DICT = 11,
    EFLR_RESERVED = 12 // for marking
};

// Defined in dlis.c
extern const char * EFLR_TYPE_NAMES[];
extern const char * IFLR_TYPE_NAMES[];
extern const char * EFLR_COMP_ROLE_NAMES[];

enum lrs_eflr_component_role_e {
    EFLR_COMP_ABSATR = 0,
    EFLR_COMP_ATTRIB = 1,
    EFLR_COMP_INVATR = 2,
    EFLR_COMP_OBJECT = 3,
    EFLR_COMP_RESERVED = 4,
    EFLR_COMP_RDSET = 5,
    EFLR_COMP_RSET = 6,
    EFLR_COMP_SET = 7,
    EFLR_COMP_MAX_ROLE = 8
};

enum lrs_structure_e {
    IFLR = 0,
    EFLR = 1
};

enum parse_state_code_e {
    EXPECTING_SUL = 0,
    EXPECTING_VR = 1,
    EXPECTING_LRS = 2,
    EXPECTING_ENCRYPTION_PACKET = 3,
    EXPECTING_EFLR_COMP = 4,
    EXPECTING_EFLR_COMP_SET = 5,
    EXPECTING_EFLR_COMP_RSET = 6,
    EXPECTING_EFLR_COMP_RDSET = 7,
    EXPECTING_EFLR_COMP_ABSATR = 8,
    EXPECTING_EFLR_COMP_ATTRIB = 9,
    EXPECTING_EFLR_COMP_INVATR = 10,
    EXPECTING_EFLR_COMP_OBJECT = 11,
    EXPECTING_LRS_TRAILING = 12,
    EXPECTING_EFLR_COMP_ATTRIB_VALUE = 13,
    EXPECTING_IFLR_HEADER = 14,
    EXPECTING_IFLR_DATA = 15
};

typedef struct eflr_template_object_s {
    sized_str_t label;
    unsigned int repcode;
    unsigned int count;
    sized_str_t unit;
    value_t default_value;
} eflr_template_object_t;

typedef enum lrs_iflr_type_e lrs_iflr_type_t;
typedef enum lrs_eflr_type_e lrs_eflr_type_t;

typedef enum lrs_structure_e lrs_structure_t;

typedef enum parse_state_code_e parse_state_code_t;
#define MAX_TEMPLT_OBJS 100

typedef struct channel_s {
    int index;// index of channel in frame
    unsigned int seq_num;
    unsigned int origin;
    unsigned int copy_number;
    char name[100];
    int dimension;
    int repcode;
	FILE *fp;
    struct channel_s * next; //next channel in all channel list
    struct channel_s * f_next; //next channel of a frame
} channel_t;

typedef struct frame_s {
    unsigned int seq_num;
    unsigned int origin;
    unsigned int copy_number;
    bool index_type; // false: using frame index, true: using first channel 
    char name[100];
    double index_min;
    double index_max;
    channel_t* channels;
    channel_t* current_channel;
    struct frame_s* next;
} frame_t;


struct parse_state_s {
    parse_state_code_t code;
    
    // data related to parsing visibleRecord
    int vr_len;
    int vr_byte_cnt;
    // ....

    // data related to parsing logicalRecordSegment
    int lrs_len;
    int lrs_byte_cnt;
    byte_t lrs_attr;
    int lrs_type;
    int lrs_trail_len;

    // data related to eflr component
    byte_t eflr_comp_first_byte;
    eflr_template_object_t templt[MAX_TEMPLT_OBJS];
    int templt_write_idx;
    int templt_read_idx;
    int attrib_value_cnt;
    int attrib_repcode;
    sized_str_t attrib_label;

    char parsing_set_type[256];
    obname_t parsing_obj;
    binn* parsing_obj_binn; //current parsing eflr object
    binn* parsing_obj_values; //the list of current object's values

    //data related to iflr data
    int parsing_repcode;
    int parsing_dimension;
    int parsing_value_cnt;
    binn* parsing_iflr_values;
    double parsing_frame_index; 

    int unparsed_buff_len;
    unsigned int seq_num; 
    byte_t unparsed_buff[100000];
};
typedef struct parse_state_s parse_state_t;

#define DLIS_BUFF_SIZE (64*1024)
#define DLIS_BUFF_NUM 4
struct dlis_s {
    int buffer_idx;
    int byte_idx;
    int max_byte_idx;

    int vr_idx;
    int lr_idx;
    int lrs_idx;

    parse_state_t parse_state;

    byte_t buffer[DLIS_BUFF_NUM][DLIS_BUFF_SIZE];

    void (*on_sul_f)(int seq, char *version, char *structure, int max_rec_len, char *ssi);
    void (*on_visible_record_header_f)(int vr_idx, int vr_len, int version);
    void (*on_logical_record_begin_f)(int lrs_idx, int lrs_len, byte_t lrs_attr, int lrs_type);
    void (*on_logical_record_end_f)(int lr_idx);
	void (*on_eflr_component_set_f)(struct dlis_s* dlis, sized_str_t *type, sized_str_t *type_len);
	void (*on_eflr_component_object_f)(struct dlis_s* dlis, obname_t obname);
    void (*on_eflr_component_attrib_f)(struct dlis_s* dlis, long count, int repcode, sized_str_t *unit);
    void (*on_eflr_component_attrib_value_f)(struct dlis_s* dlis, sized_str_t* label, value_t *val);
    void (*on_iflr_header_f)(struct dlis_s* dlis, obname_t* name, uint32_t index);
    void (*on_iflr_data_f) (struct dlis_s* dlis);
    
    
    void *sender;
    void *context;

    frame_t frames;
    frame_t* current_frame;
    channel_t channels;
    channel_t* current_channel;

    char out_dir[4000];
};
typedef struct dlis_s dlis_t;

typedef struct args_s {
    char fname[256];
    char data_dir[4000];
    void* context;
} args_t;

/* dlis file functions */
void dlis_init(dlis_t *dlis);
void* dlis_read(dlis_t *dlis, byte_t *in_buff, int in_count);

void *do_parse(void *arguments);
//int jscall(char *buff, int len);
//void initSocket();
int jscall(dlis_t* dlis, char *buff, int len);
void initSocket(dlis_t* dlis);
void write_to_curve_file(FILE* file, double index, binn* obj);
#endif
