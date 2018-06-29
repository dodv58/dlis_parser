#ifndef __DLIS_H__
#define __DLIS_H__

typedef unsigned char byte_t;
typedef struct dlis_s dlis_t;

typedef enum lrs_iflr_type_e lrs_iflr_type_t;
typedef enum lrs_eflr_type_e lrs_eflr_type_t;

typedef enum lrs_structure_e lrs_structure_t;

typedef enum parse_state_code_e parse_state_code_t;
typedef struct parse_state_s parse_state_t;
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
    EXPECTING_EFLR_COMP = 3
};

struct parse_state_s {
    parse_state_code_t code;
    
    // data related to parsing visibleRecord
    int vr_len;
    // ....

    // data related to parsing logicalRecordSegment
    int lrs_len;
    int lrs_byte_cnt;
    byte_t lrs_attr;
    int lrs_type;

    // data related to eflr component
    byte_t eflr_comp_first_byte;
    
    /*
    int is_eflr;
    int lr_begin;
    int lr_end;
    int lr_encrypted;
    int lr_encrypted_packet;
    int has_checksum;
    int has_trailing_len;
    int has_padding;
    */
};

#define DLIST_BUFF_SIZE (64*1024)
#define DLIST_BUFF_NUM 4
struct dlis_s {
    int buffer_idx;
    int byte_idx;
    int max_byte_idx;

    int vr_idx;
    int lr_idx;
    int lrs_idx;

    parse_state_t parse_state;

    byte_t buffer[DLIST_BUFF_NUM][DLIST_BUFF_SIZE];

    void (*on_sul_f)(int seq, char *version, char *structure, int max_rec_len, char *ssi);
    void (*on_visible_record_header_f)(int vr_idx, int vr_len, int version);
    void (*on_visible_record_end_f)(int vr_idx);
    void (*on_logical_record_begin_f)(int lrs_len, byte_t lrs_attr, int lrs_type);
    void (*on_logical_record_end_f)(int lr_idx);

    void (*on_eflr_f)(int lrs_idx,
            int is_start, int is_end, int remain_len,
            lrs_eflr_type_t type, byte_t attrib, byte_t *data, int data_len);

    void (*on_iflr_f)(int lrs_idx,
            int is_start, int is_end, int remain_len,
            lrs_iflr_type_t type, byte_t attrib, byte_t *data, int data_len);
};

/* dlis file functions */
void dlis_init(dlis_t *dlis);
void dlis_read(dlis_t *dlis, byte_t *in_buff, int in_count);

int parse_ushort(byte_t *data);
int parse_unorm(byte_t *data);
#endif
