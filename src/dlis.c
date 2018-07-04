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

const char * PARSE_STATE_NAMES[] = {
    "EXPECTING_SUL",
    "EXPECTING_VR",
    "EXPECTING_LRS",
    "EXPECTING_EFLR_COMP",
    "EXPECTING_EFLR_COMP_SET",
    "EXPECTING_EFLR_COMP_RSET",
    "EXPECTING_EFLR_COMP_RDSET",
    "EXPECTING_EFLR_COMP_ABSATR",
    "EXPECTING_EFLR_COMP_ATTRIB",
    "EXPECTING_EFLR_COMP_INVATR",
    "EXPECTING_EFLR_COMP_OBJECT"
};
// internal functions 
void parse(dlis_t *dlis);
int parse_sul(dlis_t *dlis);
int parse_vr_header(dlis_t *dlis, uint32_t *vr_len);
int parse_lrs_header(dlis_t *dlis, uint32_t *lrs_len, byte_t *lrs_attr, uint32_t *lrs_type);

void next_state(dlis_t* dlis);

int parse_eflr_component(dlis_t *dlis, byte_t *eflr_comp_first_byte);
int parse_eflr_component_set(dlis_t *dlis);
int parse_eflr_component_rset(dlis_t *dlis);
int parse_eflr_component_rdset(dlis_t *dlis);
int parse_eflr_component_absatr(dlis_t *dlis);
int parse_eflr_component_attrib(dlis_t *dlis);
int parse_eflr_component_invatr(dlis_t *dlis);
int parse_eflr_component_object(dlis_t *dlis);

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

int eflr_comp_attr_has_label(parse_state_t *state);
int eflr_comp_attr_has_count(parse_state_t *state);
int eflr_comp_attr_has_repcode(parse_state_t *state);
int eflr_comp_attr_has_unit(parse_state_t *state);
int eflr_comp_attr_has_value(parse_state_t *state);
int eflr_comp_set_has_type(parse_state_t *state);
int eflr_comp_set_has_name(parse_state_t *state);
int eflr_comp_object_has_name(parse_state_t *state);

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

int parse_sul(dlis_t *dlis) {
    if (dlis->max_byte_idx - dlis->byte_idx < SUL_LEN) return -1;
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

    dlis->on_sul_f(seq_number, version, structure, max_record_length, ssi);
    return SUL_LEN;
}

int parse_vr_header(dlis_t *dlis, uint32_t *vr_len) {
    if (dlis->max_byte_idx - dlis->byte_idx < VR_HEADER_LEN) 
        return -1;
    int current_byte_idx = dlis->byte_idx;
    byte_t *p_buffer = dlis->buffer[dlis->buffer_idx];

    // parse vrlen
    parse_unorm(&(p_buffer[current_byte_idx]), vr_len);
    current_byte_idx += 2;
    
    // get marker byte
    if (p_buffer[current_byte_idx] != 0xFF) {
        fprintf(stderr, 
            "Invalid visible record header: expecting 0xFF but we got 0x%x\n", 
            p_buffer[current_byte_idx]);
        exit(-1);
    }
    current_byte_idx++;

    // parse version
    uint32_t version = 0;
	parse_ushort(&(p_buffer[current_byte_idx]), &version);
    current_byte_idx++;
    
    dlis->on_visible_record_header_f(dlis->vr_idx, *vr_len, version);
    return VR_HEADER_LEN;
}

int parse_lrs_header(dlis_t *dlis, uint32_t *lrs_len, byte_t *lrs_attr, uint32_t *lrs_type) {
    if (dlis->max_byte_idx - dlis->byte_idx < LRS_HEADER_LEN) 
            return -1;
    parse_state_t *state = &(dlis->parse_state);
    int current_byte_idx = dlis->byte_idx;
    byte_t * p_buffer = dlis->buffer[dlis->buffer_idx];

    // parse lrs length
    parse_unorm(&(p_buffer[current_byte_idx]), lrs_len);
    current_byte_idx += 2;

    // parse lrs attributes
    *lrs_attr = p_buffer[current_byte_idx];
    current_byte_idx++;

    // parse lrs type
    parse_ushort(&(p_buffer[current_byte_idx]), lrs_type);
    current_byte_idx++;

    /*
    if (lrs_attr_is_eflr(state)) {
        printf("is EFLR\n");
    }
    else {
        printf("is NOT EFLR\n");
    }
    if (lrs_attr_is_first_lrs(state)) {
        printf("is the first LRS\n");
        dlis->on_logical_record_begin_f(*lrs_len, *lrs_attr, *lrs_type);
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
    */

    return current_byte_idx - dlis->byte_idx;
}
int parse_eflr_component(dlis_t *dlis, byte_t *eflr_comp_first_byte){
    if (dlis->max_byte_idx - dlis->byte_idx < 1) return -1;
    byte_t *p_buffer = dlis->buffer[dlis->buffer_idx];
    parse_state_t *state = &(dlis->parse_state);

    // parse component role
    *eflr_comp_first_byte = p_buffer[dlis->byte_idx];

    //// update state
    //dlis->byte_idx++;
    //state->eflr_comp_first_byte = first_byte;
    //state->lrs_byte_cnt++;
    return 1;
}

int parse_eflr_component_set(dlis_t *dlis) {
    int current_byte_idx = dlis->byte_idx;
    uint32_t count = 0;
    int avail_bytes = 0;
    byte_t *p_buffer = dlis->buffer[dlis->buffer_idx];

    if(!eflr_comp_set_has_type(&dlis->parse_state)) {
        fprintf(stderr, "The set component does not has type");
        exit(-1);
    }
    //parse comp set type  
    sized_str_t type;
    avail_bytes = dlis->max_byte_idx - current_byte_idx;
    if (avail_bytes < 1) return -1;
    int type_len = parse_ident(&(p_buffer[current_byte_idx]), avail_bytes, &type);
    if (type_len <= 0) {
        return -1;
    }
    // parse comp set type success
    print_str(type);
    current_byte_idx += type_len;
    
    if(eflr_comp_set_has_name(&dlis->parse_state)) {
        //parse comp set name
        sized_str_t name;
        avail_bytes = dlis->max_byte_idx - current_byte_idx;
        if (avail_bytes < 1) return -1;
        int name_len = parse_ident(&(p_buffer[current_byte_idx]), avail_bytes, &name);
        if (name_len <= 0) {
            return -1;
        }
        // parse comp set name success
        print_str(name);
        current_byte_idx += name_len;
    }
    return current_byte_idx - dlis->byte_idx;
}

int parse_eflr_component_rset(dlis_t *dlis) {
    return 0;
}
int parse_eflr_component_rdset(dlis_t *dlis) {
    return 0;
}
int parse_eflr_component_absatr(dlis_t *dlis) {
    return 0;
}
int parse_eflr_component_attrib(dlis_t *dlis) {
    printf("\teflr_comp_is_attrib: %d\n", eflr_comp_is_attrib(&dlis->parse_state));
    int current_byte_idx = dlis->byte_idx;
    uint32_t count = 1;
    uint32_t repcode = 19;
    int avail_bytes = 0;
    byte_t *p_buffer = dlis->buffer[dlis->buffer_idx];
    if (eflr_comp_attr_has_label(&dlis->parse_state)) {
        printf("eflr_comp_attr_has_label() ");
		sized_str_t lstr;
        avail_bytes = dlis->max_byte_idx - current_byte_idx;
        if (avail_bytes < 1) return -1;
        int label_len = parse_ident(&(p_buffer[current_byte_idx]), avail_bytes, &lstr);
        if (label_len <= 0) {
            return -1;
        }
        // parse label success
        print_str(lstr);
        current_byte_idx += label_len;
    }
    if (eflr_comp_attr_has_count(&dlis->parse_state)) {
        printf("eflr_comp_attr_has_count() ");
        avail_bytes = dlis->max_byte_idx - current_byte_idx;
        if (avail_bytes < 1) return -1;
        int count_len = parse_uvari(&(p_buffer[current_byte_idx]), dlis->max_byte_idx - current_byte_idx, &count);
        if(count_len <= 0) {
            return -1;
        } 
        // parse count success
        printf("%d\n", count);
        current_byte_idx += count_len;
    }
    if(eflr_comp_attr_has_repcode(&dlis->parse_state)){
        printf("eflr_comp_attr_has_repcode() ");
        avail_bytes = dlis->max_byte_idx - current_byte_idx;
        if (avail_bytes < 1) return -1;
       	parse_ushort(&p_buffer[current_byte_idx], &repcode);
        // parse repcode success
        printf("%d\n", repcode);
        current_byte_idx += 1; 
    }
    if(eflr_comp_attr_has_unit(&dlis->parse_state)){
        printf("eflr_comp_attr_has_unit() ");
        sized_str_t unit;
        avail_bytes = dlis->max_byte_idx - current_byte_idx;
        if (avail_bytes < 1) return -1;
        int unit_len = parse_ident(&p_buffer[current_byte_idx], dlis->max_byte_idx - current_byte_idx, &unit);
        if(unit_len <= 0) {
            printf("not enough data to parse component units");
            return -1;
        }
        // parse unit success
        print_str(unit);
        current_byte_idx += unit_len;
    }
    if(eflr_comp_attr_has_value(&dlis->parse_state)){
        printf("eflr_comp_attr_has_value()\n");
        avail_bytes = dlis->max_byte_idx - current_byte_idx;
        if (avail_bytes < 1) return -1;
        int nbytes_read = parse_values(&(p_buffer[current_byte_idx]), avail_bytes, count, repcode);
        if(nbytes_read <= 0) exit(-1);
		current_byte_idx += nbytes_read;
    }
    return current_byte_idx - dlis->byte_idx;
}
int parse_eflr_component_invatr(dlis_t *dlis) {
    return 0;
}
int parse_eflr_component_object(dlis_t *dlis) {
    if(!eflr_comp_object_has_name(&dlis->parse_state)){
        fprintf(stderr, "The component object does not has name");
        exit(-1);
    } 
    byte_t *p_buffer = dlis->buffer[dlis->buffer_idx];
    int current_byte_idx = dlis->byte_idx;

    obname_t obname;
    int obname_len = parse_obname(&p_buffer[current_byte_idx], dlis->max_byte_idx - current_byte_idx, &obname);
    if(obname_len <= 0) return -1;
    //parse object success
    printf("\tparse_eflr_component_object() onname_len %d\n", obname_len);
    print_obname(&obname);

    return obname_len;
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

int eflr_comp_attr_has_label(parse_state_t *state) {
    return (state->eflr_comp_first_byte & 0x10); //0001 0000
}
int eflr_comp_attr_has_count(parse_state_t *state) {
    return (state->eflr_comp_first_byte & (0x10 >> 1));
}
int eflr_comp_attr_has_repcode(parse_state_t *state) {
    return (state->eflr_comp_first_byte & (0x10 >> 2));
}
int eflr_comp_attr_has_unit(parse_state_t *state) {
    return (state->eflr_comp_first_byte & (0x10 >> 3));
}
int eflr_comp_attr_has_value(parse_state_t *state) {
    return (state->eflr_comp_first_byte & (0x10 >> 4));
}
int eflr_comp_set_has_type(parse_state_t *state){
    return (state->eflr_comp_first_byte & (0x10));
}
int eflr_comp_set_has_name(parse_state_t *state){
    return (state->eflr_comp_first_byte & (0x08));
}
int eflr_comp_object_has_name(parse_state_t *state){
    return (state->eflr_comp_first_byte & (0x10));
}

void next_state(dlis_t* dlis){
	printf("--next_state(): vr_len %d, vr_byte_cnt %d, lrs_len %d, lrs_byte_cnt %d\n", 
		dlis->parse_state.vr_len, dlis->parse_state.vr_byte_cnt, dlis->parse_state.lrs_len, dlis->parse_state.lrs_byte_cnt);
	switch(dlis->parse_state.code){
		case EXPECTING_SUL:
        	dlis->parse_state.code = EXPECTING_VR;
			break;
		case EXPECTING_VR:
			dlis->parse_state.code = EXPECTING_LRS;
			break;
		case EXPECTING_LRS:
			if (lrs_attr_is_eflr(&dlis->parse_state)) {
				dlis->parse_state.code = EXPECTING_EFLR_COMP;
			}
			else {
				// TODO: IFLR case
			}
			break;
		case EXPECTING_EFLR_COMP:
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
		case EXPECTING_EFLR_COMP_RSET:
		case EXPECTING_EFLR_COMP_RDSET:
		case EXPECTING_EFLR_COMP_ATTRIB:
		case EXPECTING_EFLR_COMP_ABSATR:
		case EXPECTING_EFLR_COMP_INVATR:
		case EXPECTING_EFLR_COMP_OBJECT:
			if (dlis->parse_state.lrs_len > dlis->parse_state.lrs_byte_cnt) {
				dlis->parse_state.code = EXPECTING_EFLR_COMP;
			}
			else if(dlis->parse_state.vr_len > dlis->parse_state.vr_byte_cnt){
				dlis->parse_state.code = EXPECTING_LRS;
			} else {
				dlis->parse_state.code = EXPECTING_VR;
			}
			break;
	}
}
void parse(dlis_t *dlis) {
    // vr related variables
    uint32_t vr_len;
    // lrs related variables
    uint32_t lrs_len, lrs_type;
    byte_t lrs_attr;
    byte_t eflr_comp_first_byte;

    int b_idx = dlis->buffer_idx;
    int len;
    byte_t *buffer = dlis->buffer[b_idx];
    if (dlis->byte_idx == dlis->max_byte_idx) return;

    while (1) {
        printf("parse loop: parse_state.code:%s, max_byte_idx:%d, byte_idx:%d\n", 
            PARSE_STATE_NAMES[dlis->parse_state.code], dlis->max_byte_idx, dlis->byte_idx);

        switch(dlis->parse_state.code) {
            case EXPECTING_SUL:
                len = parse_sul(dlis);
                if (len < 0) goto end_loop;
                // update state
                dlis->byte_idx += len;
				next_state(dlis);
                break;
            case EXPECTING_VR:
                len = parse_vr_header(dlis, &vr_len);
                if(len < 0) goto end_loop;
                // update state
                dlis->byte_idx += len;
                dlis->parse_state.vr_len = vr_len;
                dlis->parse_state.vr_byte_cnt = len;
				next_state(dlis);
                break;
            case EXPECTING_LRS:
                len = parse_lrs_header(dlis, &lrs_len, &lrs_attr, &lrs_type);
                if(len < 0) goto end_loop;

                // update state
                dlis->byte_idx += len;
                dlis->parse_state.lrs_len = lrs_len;
                //dlis->parse_state.lrs_byte_cnt = 0;
                dlis->parse_state.lrs_byte_cnt = len;
                dlis->parse_state.lrs_attr = lrs_attr;
                dlis->parse_state.lrs_type = lrs_type;
                dlis->parse_state.vr_byte_cnt += lrs_len;
				next_state(dlis);
                break;
            case EXPECTING_EFLR_COMP:
                len = parse_eflr_component(dlis, &eflr_comp_first_byte); // parse first byte of eflr component
                if (len < 0) goto end_loop;

                // update state
                dlis->parse_state.eflr_comp_first_byte = eflr_comp_first_byte;
                dlis->byte_idx += len;
                dlis->parse_state.lrs_byte_cnt += len;
				next_state(dlis);
                break;
            case EXPECTING_EFLR_COMP_SET:
            case EXPECTING_EFLR_COMP_RSET:
            case EXPECTING_EFLR_COMP_RDSET:
                len =  parse_eflr_component_set(dlis);
                if(len < 0) goto end_loop;
                // update state
                dlis->byte_idx += len;
                dlis->parse_state.lrs_byte_cnt += len;
				next_state(dlis);
                break;
            case EXPECTING_EFLR_COMP_ABSATR:
				next_state(dlis);
                break;
            case EXPECTING_EFLR_COMP_ATTRIB:
            case EXPECTING_EFLR_COMP_INVATR:
                len = parse_eflr_component_attrib(dlis);
                if(len < 0) goto end_loop;
                // update state
                dlis->byte_idx += len;
                dlis->parse_state.lrs_byte_cnt += len;
				next_state(dlis);
                break;
            case EXPECTING_EFLR_COMP_OBJECT:
                len = parse_eflr_component_object(dlis);
                if(len < 0) goto end_loop;
                // update status
                dlis->byte_idx += len;
                dlis->parse_state.lrs_byte_cnt += len;
				next_state(dlis);
                break;
    
        }
        //usleep(500*1000);
    }
end_loop:
    //usleep(500*1000);
    return;
}

