#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include "dlis.h"
#include "common.h"

void parse(dlis_t *dlis);

int parse_ushort(byte_t *data) {
    return (int)*data;
}
int parse_unorm(byte_t *data) {
    int high = (int)data[0];
    int low = (int)data[1];
    int result = low + (high<<8);
    printf("low:%d high:%d result:%d\n", low, high, result);
    return low + (high<<8);
}

void on_sul_default(int seq, char *version, char *structure, int max_rec_len, char *ssi) {
    printf("seq:%d,version:%s,structure:%s,max_rec_len:%d,ssi:%s|\n", seq, version, structure, max_rec_len, ssi);
}

void on_visible_record_begin_default(int vr_idx, int vr_len, int *version) {
    printf("--Visible Record--\n");
    printf("vr_idx: %d\n", vr_idx);
    printf("vr_len: %d\n", vr_len);
    hexDump("version:", version, sizeof(version));
}
void on_visible_record_end_default(int vr_idx) {
    printf("--Visible Record End--\n");
    printf("vr_idx: %d\n", vr_idx);
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
    dlis->on_visible_record_begin_f = &on_visible_record_begin_default;
    dlis->on_visible_record_end_f = &on_visible_record_end_default;
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
void parse_lrs_attr(byte_t attr, int *lr_eflr, int *lr_begin, int *lr_end, 
    int *lr_encrypted, int *lr_encrypted_packet, int *has_checksum, 
    int *has_trailing_len, int *has_padding) 
{
    *lr_begin = attr & 0x01;
    *lr_end = attr & 0x02;
    /* TODO
    *lr_encrypted = attr & 0x04;
    *has_encrypted_packet = attr & ;
    *has_checksum = attr & ;
    *has_trailing_len =  attr & ;
    *has_padding = attr & ;
    */
}
#define SUL_LEN 80
#define VR_HEADER_LEN 4
#define LRS_HEADER_LEN 4

void parse_sul(dlis_t *dlis) {
    printf("parse_sul\n");
    byte_t *p_buffer = dlis->buffer[dlis->buffer_idx];
    byte_t *sul = &(p_buffer[dlis->byte_idx]);

    // Check & Process SUL
    hexDump("--SUL--\n", sul, SUL_LEN);
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
    byte_t *vr_header = &(p_buffer[dlis->byte_idx]);
    // parse vrlen
    hexDump("-- VR_HEADER --", vr_header, VR_HEADER_LEN);
    int vr_len = parse_unorm(&(dlis->buffer[dlis->buffer_idx][dlis->byte_idx]));
    dlis->byte_idx += 2;
    // parse version
    dlis->byte_idx++;
    int version = parse_ushort(&(dlis->buffer[dlis->buffer_idx][dlis->byte_idx++]));
    printf("vr_len:%d, version:%d\n", vr_len, version);
    dlis->on_visible_record_begin_f(dlis->vr_idx, vr_len, &version);
}

void parse_lrs_header(dlis_t *dlis) {
    parse_state_t *state = &(dlis->parse_state);
    // parse lrs length
    state->lrs_len = parse_unorm(&(dlis->buffer[dlis->buffer_idx][dlis->byte_idx]));
    state->lrs_byte_cnt = 0;
    dlis->byte_idx += 2;
    
    byte_t attr = dlis->buffer[dlis->buffer_idx][dlis->byte_idx];

    parse_lrs_attr(attr, &state->is_eflr, &state->lr_begin, &state->lr_end, 
        &state->lr_encrypted, &state->lr_encrypted_packet, &state->has_checksum, 
        &state->has_trailing_len, &state->has_padding);

    if (state->lr_begin) {
        if (dlis->lr_idx % 2) {
            fprintf(stderr, "Unexpected lr_idx\n");
            exit(-1);
        }
        dlis->on_logical_record_begin_f(dlis->lr_idx/2);
        dlis->lr_idx++;
        dlis->lrs_idx = 0;
    }
}
void parse_lrs_body(dlis_t *dlis) {
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
                printf("parse vr success\n");
                exit(0);
                if (dlis->max_byte_idx - dlis->byte_idx < LRS_HEADER_LEN) 
                    goto end_loop;
                parse_lrs_header(dlis);
                dlis->parse_state.code = LRS_INPROG;
                break;
            case LRS_INPROG:
                if (dlis->max_byte_idx - dlis->byte_idx < dlis->parse_state.lrs_len) {
                    
                }
                else {
                }
                break;
        }
    }
end_loop:
    return;
}
