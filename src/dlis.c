#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include "dlis.h"
#include "common.h"

const char * EFLR_TYPE_NAMES[] = {
    "FHRL", "OLR", "AXIS", "CHANNL", "FRAME", "STATIC", "SCRIPT", "UPDATE", "UDI", "LNAME",
    "SPEC", "DICT"
};

const char * IFLR_TYPE_NAMES[] = {
    "FDATA", "NOFORMAT"
};

const char * EFLR_COMP_ROLE_NAMES[] = {
    "ABSATR",
    "ATTRIB",
    "INVATR",
    "OBJECT",
    "RESERVED",
    "RDSET",
    "RSET",
    "SET"
};
// internal functions 
void parse(dlis_t *dlis);
void parse_sul(dlis_t *dlis);
void parse_vr_header(dlis_t *dlis);
void parse_lrs_header(dlis_t *dlis);

void parse_eflr_component(dlis_t *dlis);
int parse_eflr_component_set(dlis_t *dlis);
void parse_eflr_component_rset(dlis_t *dlis);
void parse_eflr_component_rdset(dlis_t *dlis);
void parse_eflr_component_absatr(dlis_t *dlis);
void parse_eflr_component_attrib(dlis_t *dlis);
void parse_eflr_component_invatr(dlis_t *dlis);
void parse_eflr_component_object(dlis_t *dlis);

int lrs_attr_is_eflr(parse_state_t *state);
int lrs_attr_is_first_lrs(parse_state_t *state);

int lrs_attr_is_last_lrs(parse_state_t *state);
int lrs_attr_is_encrypted(parse_state_t *state);
int lrs_attr_has_encryption_packet(parse_state_t *state);
int lrs_attr_has_checksum(parse_state_t *state);
int lrs_attr_has_trailing(parse_state_t *state);
int lrs_attr_has_padding(parse_state_t *state);
const char *lrs_type_get_name(parse_state_t *state);

int eflr_comp_is_set(parse_state_t *state);
int eflr_comp_is_rset(parse_state_t *state);
int eflr_comp_is_rdset(parse_state_t *state);
int eflr_comp_is_absatr(parse_state_t *state);
int eflr_comp_is_attrib(parse_state_t *state);
int eflr_comp_is_invatr(parse_state_t *state);
int eflr_comp_is_object(parse_state_t *state);

int eflr_comp_has_label(parse_state_t *state);
int eflr_comp_has_count(parse_state_t *state);
int eflr_comp_has_repcode(parse_state_t *state);
int eflr_comp_has_unit(parse_state_t *state);
int eflr_comp_has_value(parse_state_t *state);

void on_sul_default(int seq, char *version, char *structure, int max_rec_len, char *ssi) {
    printf("--SUL--\n");
    printf("seq:%d,version:%s,structure:%s,max_rec_len:%d,ssi:%s|\n", seq, version, structure, max_rec_len, ssi);
}

void on_visible_record_header_default(int vr_idx, int vr_len, int version) {
    printf("--Visible Record--\n");
    printf("vr_idx:%d, vr_len:%d, version:%d\n", vr_idx, vr_len, version);
}
void on_visible_record_end_default(int vr_idx) {
    printf("--Visible Record End--\n");
    printf("vr_idx: %d\n", vr_idx);
}
void on_logical_record_begin_default(int lrs_len, byte_t lrs_attr, int lrs_type) {
    printf("--Logical record begin-- %d, %d, %d\n", lrs_len, lrs_attr, lrs_type);
}
void dlis_init(dlis_t *dlis) {
    printf("dlis init\n");
    dlis->buffer_idx = 0;
    dlis->byte_idx = 0;
    dlis->max_byte_idx = 0;
    dlis->vr_idx = 0;
    dlis->lr_idx = 0;
    dlis->lrs_idx = 0;

    dlis->parse_state.code = EXPECTING_SUL;
    //bzero(dlis, sizeof(dlis_t));
    dlis->on_sul_f = &on_sul_default;
    dlis->on_visible_record_header_f = &on_visible_record_header_default;
    dlis->on_visible_record_end_f = &on_visible_record_end_default;
    dlis->on_logical_record_begin_f = &on_logical_record_begin_default;
    //..........
}
void dlis_read(dlis_t *dlis, byte_t *in_buff, int in_count) {
    printf("dlis_read: in_count:%d, max_byte_idx:%d\n", in_count, dlis->max_byte_idx);
    int b_idx = dlis->buffer_idx;
    byte_t *buffer = dlis->buffer[b_idx];
    memcpy(&(buffer[dlis->max_byte_idx]), in_buff, in_count);
    dlis->max_byte_idx += in_count;
    parse(dlis);
}

#define SUL_LEN 80
#define VR_HEADER_LEN 4
#define LRS_HEADER_LEN 4

void parse_sul(dlis_t *dlis) {
    printf("parse_sul\n");
    byte_t *p_buffer = dlis->buffer[dlis->buffer_idx];
    byte_t *sul = &(p_buffer[dlis->byte_idx]);

    // Check & Process SUL
    //hexDump("--SUL--\n", sul, SUL_LEN);
    char seq[20];
    char version[20];
    char structure[20];
    char max_rec_len[20];
    char ssi[80];
    trim(seq, 4, (const char *)sul);
    trim(version, 5, (const char *)(sul + 4));
    trim(structure, 6, (const char *)(sul + 9));
    trim(max_rec_len, 5, (const char *)(sul + 15));
    trim(ssi, 60, (const char *)(sul + 20));
    
    // check SUL
    if (!is_integer(seq, strlen(seq)) || !is_integer(max_rec_len, strlen(max_rec_len))) {
        fprintf(stderr, "Not a valid dlis file (invalid SUL)\n");
        exit(-1);
    }

    int seq_number = strtol(seq, NULL, 10);
    int max_record_length = strtol(max_rec_len, NULL, 10);
    dlis->byte_idx += SUL_LEN;

    dlis->on_sul_f(seq_number, version, structure, max_record_length, ssi);
}

void parse_vr_header(dlis_t *dlis) {
    /*
    if (dlis->vr_idx % 2) {
        fprintf(stderr, "Unexpected\n");
        exit(-1);
    }
    int vr_idx = dlis->vr_idx/2;
    dlis->vr_idx++;
    */
    byte_t *p_buffer = dlis->buffer[dlis->buffer_idx];

    // parse vrlen
    int vr_len = parse_unorm(&(p_buffer[dlis->byte_idx]));
    dlis->byte_idx += 2;
    
    // get marker byte
    if (p_buffer[dlis->byte_idx] != 0xFF) {
        fprintf(stderr, 
            "Invalid visible record header: expecting 0xFF but we got 0x%x\n", 
            p_buffer[dlis->byte_idx]);
        exit(-1);
    }
    dlis->byte_idx++;

    // parse version
    int version = parse_ushort(&(p_buffer[dlis->byte_idx]));
    dlis->byte_idx++;
    
    // update parsing state
    parse_state_t *state = &(dlis->parse_state);
    state->vr_len = vr_len;
    
    dlis->on_visible_record_header_f(dlis->vr_idx, vr_len, version);
}

void parse_lrs_header(dlis_t *dlis) {
    parse_state_t *state = &(dlis->parse_state);
    byte_t * p_buffer = dlis->buffer[dlis->buffer_idx];

    //hexDump("-- LRS HEADER --", &(p_buffer[dlis->byte_idx]), LRS_HEADER_LEN);
    
    // parse lrs length
    int lrs_len = parse_unorm(&(p_buffer[dlis->byte_idx]));
    dlis->byte_idx += 2;

    // parse lrs attributes
    byte_t lrs_attr = p_buffer[dlis->byte_idx];
    dlis->byte_idx++;

    // parse lrs type
    int lrs_type = parse_ushort(&(p_buffer[dlis->byte_idx]));
    dlis->byte_idx++;

   /* parse_lrs_attr(attr, &state->is_eflr, &state->lr_begin, &state->lr_end, 
        &state->lr_encrypted, &state->lr_encrypted_packet, &state->has_checksum, 
        &state->has_trailing_len, &state->has_padding);
    */

    // update parsing state
    state->lrs_len = lrs_len;
    state->lrs_byte_cnt = 0;
    state->lrs_byte_cnt += LRS_HEADER_LEN;
    state->lrs_attr = lrs_attr;
    state->lrs_type = lrs_type;

    if (lrs_attr_is_eflr(state)) {
        printf("is EFLR\n");
    }
    else {
        printf("is NOT EFLR\n");
    }
    if (lrs_attr_is_first_lrs(state)) {
        printf("is the first LRS\n");
        dlis->on_logical_record_begin_f(lrs_len, lrs_attr, lrs_type);
    }
    else {
        printf("is NOT the first LRS\n");
    }

    if (lrs_attr_is_last_lrs(state)) {
        printf("is the last LRS\n");
    }
    else {
        printf("is NOT the last LRS\n");
    }

    if (lrs_attr_is_encrypted(state)) {
        printf("is encrypted\n");
    }
    else {
        printf("is NOT encrypted\n");
    }
    if (lrs_attr_has_encryption_packet(state)) {
        printf("has encryption packet\n");
    }
    else {
        printf("has NOT encryption packet\n");
    }
    if (lrs_attr_has_checksum(state)) {
        printf("has checksum\n");
    }
    else {
        printf("has NOT checksum\n");
    }
    if (lrs_attr_has_trailing(state)) {
        printf("has trailing\n");
    }
    else {
        printf("has NOT trailing\n");
    }
    if (lrs_attr_has_padding(state)) {
        printf("has padding\n");
    }
    else {
        printf("has NOT padding\n");
    }

    printf("lrs type: %s\n", lrs_type_get_name(state));
}
void parse_eflr_component(dlis_t *dlis){
    byte_t *p_buffer = dlis->buffer[dlis->buffer_idx];
    parse_state_t *state = &(dlis->parse_state);

    // parse component role
    byte_t first_byte = p_buffer[dlis->byte_idx];
    dlis->byte_idx++;

    // update state
    state->eflr_comp_first_byte = first_byte;
    state->lrs_byte_cnt++;
}

int parse_eflr_component_set(dlis_t *dlis) {
    int current_byte_idx = dlis->byte_idx;
    int count = 0;
    int repcode = 0;
    int avail_bytes = 0;
    byte_t *p_buffer = dlis->buffer[dlis->buffer_idx];
    if (eflr_comp_has_label(&dlis->parse_state)) {
        sized_str_t lstr;
        avail_bytes = dlis->max_byte_idx - current_byte_idx;
        if (avail_bytes < 1) return -1;
        int label_len = parse_ident(&(p_buffer[current_byte_idx]), avail_bytes, &lstr);
        if (label_len <= 0) {
            return -1;
        }
        // parse indent success
        current_byte_idx += label_len;
    }
    if (eflr_comp_has_count(&dlis->parse_state)) {
        avail_bytes = dlis->max_byte_idx - current_byte_idx;
        if (avail_bytes < 1) return -1;
        int count_len = parse_uvari(&(p_buffer[current_byte_idx]), dlis->max_byte_idx - current_byte_idx, &count);
        if(count_len <= 0) {
            return -1;
        } 
        //update state
        current_byte_idx += count_len;
    }
    if(eflr_comp_has_repcode(&dlis->parse_state)){
        avail_bytes = dlis->max_byte_idx - current_byte_idx;
        if (avail_bytes < 1) return -1;
        repcode = parse_ushort(&p_buffer[current_byte_idx]);
        current_byte_idx += 1; 
    }
    if(eflr_comp_has_unit(&dlis->parse_state)){
        sized_str_t unit;
        avail_bytes = dlis->max_byte_idx - current_byte_idx;
        if (avail_bytes < 1) return -1;
        int unit_len = parse_ident(&p_buffer[current_byte_idx], dlis->max_byte_idx - current_byte_idx, &unit);
        if(unit_len <= 0) {
            printf("not enough data to parse component units");
            return -1;
        }
        current_byte_idx += unit_len;
    }
    if(eflr_comp_has_value(&dlis->parse_state)){
        avail_bytes = dlis->max_byte_idx - current_byte_idx;
        if (avail_bytes < 1) return -1;
        parse_values(&(p_buffer[current_byte_idx]), avail_bytes, count, repcode);
    }
    return 0;
}

void parse_eflr_component_rset(dlis_t *dlis) {
}
void parse_eflr_component_rdset(dlis_t *dlis) {
}
void parse_eflr_component_absatr(dlis_t *dlis) {
}
void parse_eflr_component_attrib(dlis_t *dlis) {
}
void parse_eflr_component_invatr(dlis_t *dlis) {
}
void parse_eflr_component_object(dlis_t *dlis) {
}

int lrs_attr_is_eflr(parse_state_t *state) {
    return state->lrs_attr & 0x80;
}
int lrs_attr_is_first_lrs(parse_state_t *state) {
    return !(state->lrs_attr & (0x80 >> 1));
}
int lrs_attr_is_last_lrs(parse_state_t *state) {
    return !(state->lrs_attr & (0x80 >> 2));
}
int lrs_attr_is_encrypted(parse_state_t *state){
    return state->lrs_attr & (0x80 >> 3);
}
int lrs_attr_has_encryption_packet(parse_state_t *state){
    return state->lrs_attr & (0x80 >> 4);
}
int lrs_attr_has_checksum(parse_state_t *state){
    return state->lrs_attr & (0x80 >> 5);
}
int lrs_attr_has_trailing(parse_state_t *state){
    return state->lrs_attr & (0x80 >> 6);
}
int lrs_attr_has_padding(parse_state_t *state){
    return state->lrs_attr & (0x80 >> 7);
}

const char *lrs_type_get_name(parse_state_t *state) {
    const char *name = 0;
    int type = state->lrs_type;
    if(lrs_attr_is_eflr(state)) {
        if (type < EFLR_RESERVED) {
            name = EFLR_TYPE_NAMES[type];
        }
        else {
            name = "Private EFLR";
        }
    }
    else {
        if (type < IFLR_RESERVED) {
            name = IFLR_TYPE_NAMES[type];
        }
        else if (type == EOD ){
            name = "EOD";
        }
        else {
            name = "Private IFLR";
        }
    }
    return name;
}
int eflr_comp_is_set(parse_state_t *state) {
    return (state->eflr_comp_first_byte >> 5) == EFLR_COMP_SET;
}
int eflr_comp_is_rset(parse_state_t *state) {
    return (state->eflr_comp_first_byte >> 5) == EFLR_COMP_RSET;
}
int eflr_comp_is_rdset(parse_state_t *state) {
    return (state->eflr_comp_first_byte >> 5) == EFLR_COMP_RDSET;
}
int eflr_comp_is_absatr(parse_state_t *state) {
    return (state->eflr_comp_first_byte >> 5) == EFLR_COMP_ABSATR;
}
int eflr_comp_is_attrib(parse_state_t *state) {
    return (state->eflr_comp_first_byte >> 5) == EFLR_COMP_ATTRIB;
}
int eflr_comp_is_invatr(parse_state_t *state) {
    return (state->eflr_comp_first_byte >> 5) == EFLR_COMP_INVATR;
}
int eflr_comp_is_object(parse_state_t *state) {
    return (state->eflr_comp_first_byte >> 5) == EFLR_COMP_OBJECT;
}

int eflr_comp_has_label(parse_state_t *state) {
    return (state->eflr_comp_first_byte & 0x10); //0001 0000
}
int eflr_comp_has_count(parse_state_t *state) {
    return (state->eflr_comp_first_byte & (0x10 >> 1));
}
int eflr_comp_has_repcode(parse_state_t *state) {
    return (state->eflr_comp_first_byte & (0x10 >> 2));
}
int eflr_comp_has_unit(parse_state_t *state) {
    return (state->eflr_comp_first_byte & (0x10 >> 3));
}
int eflr_comp_has_value(parse_state_t *state) {
    return (state->eflr_comp_first_byte & (0x10 >> 4));
}

void parse(dlis_t *dlis) {
    int b_idx = dlis->buffer_idx;
    byte_t *buffer = dlis->buffer[b_idx];
    if (dlis->byte_idx == dlis->max_byte_idx) return;
    while (1) {
        printf("parse loop: parse_state.code:%d, max_byte_idx:%d, byte_idx:%d\n", 
            dlis->parse_state.code, dlis->max_byte_idx, dlis->byte_idx);

        switch(dlis->parse_state.code) {
            case EXPECTING_SUL:
                if (dlis->max_byte_idx - dlis->byte_idx < SUL_LEN) 
                    goto end_loop;
                parse_sul(dlis);
                dlis->parse_state.code = EXPECTING_VR;
                break;
            case EXPECTING_VR:
                if (dlis->max_byte_idx - dlis->byte_idx < VR_HEADER_LEN) 
                    goto end_loop;
                parse_vr_header(dlis);
                dlis->parse_state.code = EXPECTING_LRS;
                break;
            case EXPECTING_LRS:
                if (dlis->max_byte_idx - dlis->byte_idx < LRS_HEADER_LEN) 
                    goto end_loop;
                parse_lrs_header(dlis);
                if (lrs_attr_is_eflr(&dlis->parse_state)) {
                    dlis->parse_state.code = EXPECTING_EFLR_COMP;
                }
                else {
                    // IFLR case
                }
                //dlis->parse_state.code = LRS_INPROG;
                break;
            case EXPECTING_EFLR_COMP:
                if (dlis->max_byte_idx - dlis->byte_idx < 1) goto end_loop;
                parse_eflr_component(dlis);
                if (eflr_comp_is_set(&(dlis->parse_state))) {
                    dlis->parse_state.code = EXPECTING_EFLR_COMP_SET;
                }
                else if (eflr_comp_is_rset(&(dlis->parse_state))) {
                    dlis->parse_state.code = EXPECTING_EFLR_COMP_RSET;
                }
                else if (eflr_comp_is_rdset(&(dlis->parse_state))) {
                    dlis->parse_state.code = EXPECTING_EFLR_COMP_RDSET;
                } 
                else if (eflr_comp_is_absatr(&(dlis->parse_state))) {
                    dlis->parse_state.code = EXPECTING_EFLR_COMP_ABSATR;
                }
                else if (eflr_comp_is_attrib(&(dlis->parse_state))) {
                    dlis->parse_state.code = EXPECTING_EFLR_COMP_ATTRIB;
                }
                else if (eflr_comp_is_invatr(&(dlis->parse_state))) {
                    dlis->parse_state.code = EXPECTING_EFLR_COMP_INVATR;
                }
                else if (eflr_comp_is_object(&(dlis->parse_state))) {
                    dlis->parse_state.code = EXPECTING_EFLR_COMP_OBJECT;
                }
                break;
            case EXPECTING_EFLR_COMP_SET:
                if( parse_eflr_component_set(dlis) < 0) goto end_loop;
                break;
            case EXPECTING_EFLR_COMP_RSET:
                break;
            case EXPECTING_EFLR_COMP_RDSET:
                break;
            case EXPECTING_EFLR_COMP_ABSATR:
                break;
            case EXPECTING_EFLR_COMP_ATTRIB:
                break;
            case EXPECTING_EFLR_COMP_INVATR:
                break;
            case EXPECTING_EFLR_COMP_OBJECT:
                break;
    
        }
        //usleep(500*1000);
    }
end_loop:
    //usleep(500*1000);
    return;
}

