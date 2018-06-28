#ifndef __DLIST_H__
#define __DLIST_H__

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
    EOD = 127,

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
    DICT = 11
};

enum lrs_structure_e {
    IFLR = 0,
    EFLR = 1
};

enum parse_state_code_e {
    EXPECTING_SUL = 0,
    EXPECTING_VR = 1,
    EXPECTING_LRS = 2,
    LRS_INPROG = 3
};

struct parse_state_s {
    parse_state_code_t code;
    int lrs_len;
    int lrs_byte_cnt;

    int is_eflr;
    int lr_begin;
    int lr_end;
    int lr_encrypted;
    int lr_encrypted_packet;
    int has_checksum;
    int has_trailing_len;
    int has_padding;
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

    void (*on_sul_f)(byte_t *sul, int len);
    void (*on_visible_record_begin_f)(int vr_idx, int vr_len, int *version);
    void (*on_visible_record_end_f)(int vr_idx);
    void (*on_logical_record_begin_f)(int lr_idx);
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
