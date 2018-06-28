#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include "dlis.h"

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
void hexDump (char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        printf ("%s:\n", desc);

    if (len == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }
    if (len < 0) {
        printf("  NEGATIVE LENGTH: %i\n",len);
        return;
    }

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}

void on_sul_default(byte_t *sul, int len) {
    hexDump("--SUL--\n", sul, len);
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
    byte_t *p_buffer = dlis->buffer[dlis->buffer_idx];
    byte_t *p_sul = &(p_buffer[dlis->byte_idx]);
    dlis->on_sul_f( p_sul, SUL_LEN );
    dlis->byte_idx += SUL_LEN;
}

void parse_vr_header(dlis_t *dlis) {
    if (dlis->vr_idx % 2) {
        fprintf(stderr, "Unexpected\n");
        exit(-1);
    }
    int vr_idx = dlis->vr_idx/2;
    dlis->vr_idx++;

    // parse vrlen
    int vr_len = parse_unorm(&(dlis->buffer[dlis->buffer_idx][dlis->byte_idx]));
    dlis->byte_idx += 2;
    // parse version
    dlis->byte_idx++;
    int version = parse_ushort(&(dlis->buffer[dlis->buffer_idx][dlis->byte_idx++]));
    dlis->on_visible_record_begin_f(vr_idx, vr_len, &version);
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
        switch(dlis->parse_state.code) {
            case EXPECTING_SUL:
                if (dlis->max_byte_idx - dlis->byte_idx < SUL_LEN) break;
                parse_sul(dlis);
                dlis->parse_state.code = EXPECTING_VR;
                break;
            case EXPECTING_VR:
                if (dlis->max_byte_idx - dlis->byte_idx < VR_HEADER_LEN) break;
                parse_vr_header(dlis);
                dlis->parse_state.code = EXPECTING_LRS;
                break;
            case EXPECTING_LRS:
                if (dlis->max_byte_idx - dlis->byte_idx < LRS_HEADER_LEN) break;
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
}
