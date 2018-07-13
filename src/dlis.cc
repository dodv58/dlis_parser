#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "dlis.h"
#include "common.h"
#include <errno.h>
#include <stdarg.h>

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
    "EXPECTING_EFLR_COMP_OBJECT",
    "EXPECTING_LRS_TRAILING",
    "EXPECTING_EFLR_COMP_ATTRIB_VALUE",
    "EXPECTING_IFLR_DATA"
};
// internal functions 
void parse(dlis_t *dlis);
int parse_sul(dlis_t *dlis);
int parse_vr_header(dlis_t *dlis, uint32_t *vr_len, uint32_t *version);
int parse_lrs_header(dlis_t *dlis, uint32_t *lrs_len, byte_t *lrs_attr, uint32_t *lrs_type);
int parse_lrs_trailing(dlis_t *dlis);

void next_state(dlis_t* dlis);

int parse_eflr_component(dlis_t *dlis, byte_t *eflr_comp_first_byte);
int parse_eflr_component_set(dlis_t *dlis, sized_str_t *type, sized_str_t *name);
int parse_eflr_component_rset(dlis_t *dlis);
int parse_eflr_component_rdset(dlis_t *dlis);
int parse_eflr_component_absatr(dlis_t *dlis);
int parse_eflr_component_attrib(dlis_t *dlis, sized_str_t *label, long *count, 
								             int *repcode, sized_str_t *unit);
int parse_eflr_component_attrib_value(dlis_t *dlis, value_t *val);
int parse_eflr_component_invatr(dlis_t *dlis);
int parse_eflr_component_object(dlis_t *dlis, obname_t * obname);

int parse_iflr_data(dlis_t *dlis);

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
void eflr_set_template_object_queue(dlis_t* dlis, sized_str_t *label, long count, int repcode, sized_str_t *unit);
void eflr_set_template_object_get(dlis_t* dlis, sized_str_t *label, long *count, int *repcode, sized_str_t *unit);
void eflr_set_template_object_dequeue(dlis_t* dlis);

char g_buff[1024];
int g_idx = 0;

void on_sul_default(int seq, char *version, char *structure, int max_rec_len, char *ssi) {
    __g_cstart(0);
    int len = _printf(__g_cbuff, __g_clen, "--> SUL: seq=%d, version=%s, structure=%s, max_rec_len=%d, ssi=%s|\n", seq, version, structure, max_rec_len, ssi);
    __g_cend(len);
    app_print(_eflr_data_, g_buff);
}

void on_visible_record_header_default(int vr_idx, int vr_len, int version) {
    __g_cstart(0);
    int len = _printf(__g_cbuff, __g_clen, "--> VR:vr_idx=%d, vr_len=%d, version=%d\n", vr_idx, vr_len, version);
    __g_cend(len);
    app_print(_eflr_data_, g_buff);
}
void on_logical_record_begin_default(int lrs_idx, int lrs_len, byte_t lrs_attr, int lrs_type) {
    __g_cstart(0);
    int len = _printf(__g_cbuff, __g_clen, "--> LRS:idx=%d, len=%d, attr=%d, type=%d\n", lrs_idx, lrs_len, lrs_attr, lrs_type);
    __g_cend(len);
    app_print(_eflr_data_, g_buff);
}
void on_logical_record_end_default(int lrs_idx) {
    __g_cstart(0);
    int len = _printf(__g_cbuff, __g_clen, "--> LRS END:idx=%d\n", lrs_idx);
    __g_cend(len);
    app_print(_eflr_data_, g_buff);
}

void on_eflr_component_set_default(sized_str_t *type, sized_str_t *name) {
    __g_cstart(0);
	int len = _printf(__g_cbuff, __g_clen, "--> SET:type=%.*s, name=%.*s\n", type->len, type->buff, name->len, name->buff);
    __g_cend(len);
    app_print(_eflr_data_, g_buff);
}
void on_eflr_component_object_default(obname_t obname){
    __g_cstart(0);
	int len = _printf(__g_cbuff, __g_clen, "--> OBJECT:");
    __g_cend(len);
    print_obname(&obname);
    len = _printf(__g_cbuff, __g_clen, "\n");
    __g_cend(len);
    app_print(_eflr_data_, g_buff);
}
void on_eflr_component_attrib_default(sized_str_t *label, long count, int repcode, sized_str_t *unit, obname_t *obname, int has_value) {
    __g_cstart(0);
	int len = _printf(__g_cbuff, __g_clen, "--> ATTRIB");
    __g_cend(len);
    print_obname(obname); 
    len = _printf(__g_cbuff, __g_clen, ":");
    __g_cend(len);
	if (label->len > 0) {
		len = _printf(__g_cbuff, __g_clen, "label=%.*s, ", label->len, label->buff);
        __g_cend(len);
	}
	if (unit->len > 0) {
		len = _printf(__g_cbuff, __g_clen, "unit=%.*s, ", unit->len, unit->buff);
        __g_cend(len);
	}
	if (count > 0) {
		len = _printf(__g_cbuff, __g_clen, "count=%ld, ", count);
        __g_cend(len);
	}
	if (repcode >= 0) {
		len = _printf(__g_cbuff, __g_clen, "repcode=%d, ", repcode);
        __g_cend(len);
	}
	len = _printf(__g_cbuff, __g_clen, "has_value:%d\n", has_value);
    __g_cend(len);
    app_print(_eflr_data_, g_buff);
}
void on_eflr_component_attrib_value_default(int repcode, value_t *val) {
    __g_cstart(0);
    int len = _printf(__g_cbuff, __g_clen, "--> ATTRIB-VALUE:");
    __g_cend(len);
	if (val->repcode > 0) {
		print_value(val);
        len = _printf(__g_cbuff, __g_clen, "\n");
        __g_cend(len);
	}
    else {
        len = _printf(__g_cbuff, __g_clen, "Unknown value type\n");
        __g_cend(len);
    }
    app_print(_eflr_data_, g_buff);
}
void on_iflr_data_default(int data_len) {
    __g_cstart(0);
    int len = _printf(__g_cbuff, __g_clen, "-->IFLR_DATA:%d\n", data_len);
    __g_cend(len);
    app_print(_eflr_data_, g_buff);
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
    dlis->on_logical_record_begin_f = &on_logical_record_begin_default;
    dlis->on_logical_record_end_f = &on_logical_record_end_default;
	dlis->on_eflr_component_set_f = &on_eflr_component_set_default;
	dlis->on_eflr_component_object_f = &on_eflr_component_object_default;
	dlis->on_eflr_component_attrib_f = &on_eflr_component_attrib_default;
    dlis->on_eflr_component_attrib_value_f = &on_eflr_component_attrib_value_default;
    dlis->on_iflr_data_f = &on_iflr_data_default;
    //..........
}
void dlis_read(dlis_t *dlis, byte_t *in_buff, int in_count) {
    //printf("dlis_read: in_count:%d, byte_idx=%d, max_byte_idx:%d, buffer_idx=%d\n", in_count, dlis->byte_idx, dlis->max_byte_idx, dlis->buffer_idx);
    int b_idx = dlis->buffer_idx;
    if (dlis->max_byte_idx + in_count >= DLIS_BUFF_SIZE) {
        dlis->buffer_idx = (b_idx + 1) % DLIS_BUFF_NUM;
        //printf("********** SWITCH BUFFER %d --> %d\n", b_idx, dlis->buffer_idx);
        // copy unparsed data to new buffer and set byte_idx & max_byte_idx properly
        byte_t *source = &dlis->buffer[b_idx][dlis->byte_idx];
        int len = dlis->max_byte_idx - dlis->byte_idx;
        byte_t *dest = dlis->buffer[dlis->buffer_idx];
        memcpy(dest, source, len);
        dlis->byte_idx = 0;
        dlis->max_byte_idx = len;
    }

    b_idx = dlis->buffer_idx;
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
	
	// Fire callback on_sul
    dlis->on_sul_f(seq_number, version, structure, max_record_length, ssi);

    return SUL_LEN;
}

int parse_vr_header(dlis_t *dlis, uint32_t *vr_len, uint32_t *vr_version) {
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
        //hexDump("----\n", &p_buffer[current_byte_idx - 2], 10);
        exit(-1);
    }
    current_byte_idx++;

    // parse version
	parse_ushort(&(p_buffer[current_byte_idx]), vr_version);
    

    return VR_HEADER_LEN;
}

int parse_lrs_header(dlis_t *dlis, uint32_t *lrs_len, byte_t *lrs_attr, uint32_t *lrs_type) {
    if (dlis->max_byte_idx - dlis->byte_idx < LRS_HEADER_LEN) 
            return -1;
    //parse_state_t *state = &(dlis->parse_state);
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

    return current_byte_idx - dlis->byte_idx;
}

int parse_lrs_trailing(dlis_t *dlis) {
    int lrs_trail_len = 0;
    if(lrs_attr_has_checksum(&dlis->parse_state)) 
        lrs_trail_len += 2;
    if(lrs_attr_has_trailing(&dlis->parse_state)) 
        lrs_trail_len += 2;
    if(lrs_attr_has_padding(&dlis->parse_state)) 
        lrs_trail_len ++;

    if (dlis->max_byte_idx - dlis->byte_idx < lrs_trail_len) 
        return -1;
    return lrs_trail_len;
}

int parse_eflr_component(dlis_t *dlis, byte_t *eflr_comp_first_byte){
    if (dlis->max_byte_idx - dlis->byte_idx < 1) return -1;
    byte_t *p_buffer = dlis->buffer[dlis->buffer_idx];
    //parse_state_t *state = &(dlis->parse_state);

    // parse component first byte (role (3-bit) & format (5-bit))
    *eflr_comp_first_byte = p_buffer[dlis->byte_idx];

    return 1;
}

int parse_eflr_component_set(dlis_t *dlis, sized_str_t *type, sized_str_t *name) {
    int current_byte_idx = dlis->byte_idx;
    int avail_bytes = 0;
    byte_t *p_buffer = dlis->buffer[dlis->buffer_idx];

    if(!eflr_comp_set_has_type(&dlis->parse_state)) {
        fprintf(stderr, "The set component does not has type");
        exit(-1);
    }
    //parse comp set type
    avail_bytes = dlis->max_byte_idx - current_byte_idx;
    if (avail_bytes < 1) return -1;
    int type_len = parse_ident(&(p_buffer[current_byte_idx]), avail_bytes, type);
    if (type_len <= 0) {
        return -1;
    }
    // parse comp set type success
    current_byte_idx += type_len;
    
    if(eflr_comp_set_has_name(&dlis->parse_state)) {
        //parse comp set name
        avail_bytes = dlis->max_byte_idx - current_byte_idx;
        if (avail_bytes < 1) return -1;
        int name_len = parse_ident(&(p_buffer[current_byte_idx]), avail_bytes, name);
        if (name_len <= 0) {
            return -1;
        }
        // parse comp set name success
        current_byte_idx += name_len;
    }
	else {
		name->buff = 0;
		name->len = 0;
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
int parse_eflr_component_attrib(dlis_t *dlis, sized_str_t *label, long *count, 
								int *repcode, sized_str_t *unit) 
{
	if (dlis->parse_state.templt_read_idx >= 0) {
		eflr_set_template_object_get(dlis, label, count, repcode, unit);
	}

	int current_byte_idx = dlis->byte_idx;
	//uint32_t count = 1;
	//uint32_t repcode = 19;
	int avail_bytes = 0;
	byte_t *p_buffer = dlis->buffer[dlis->buffer_idx];
	if (eflr_comp_attr_has_label(&dlis->parse_state)) {
		avail_bytes = dlis->max_byte_idx - current_byte_idx;
		if (avail_bytes < 1) return -1;
		int label_len = parse_ident(&(p_buffer[current_byte_idx]), avail_bytes, label);
		if (label_len <= 0) {
			return -1;
		}
		// parse label success
		current_byte_idx += label_len;
	}
	else {
		//label->len = 0;
		//label->buff = 0;
	}
	if (eflr_comp_attr_has_count(&dlis->parse_state)) {
		avail_bytes = dlis->max_byte_idx - current_byte_idx;
		if (avail_bytes < 1) return -1;
		int count_len = parse_uvari(&(p_buffer[current_byte_idx]), dlis->max_byte_idx - current_byte_idx, (unsigned int *)count);
		if(count_len <= 0) {
			return -1;
		} 
		// parse count success
		current_byte_idx += count_len;
	}
	else {
		//*count = -1;
	}
	if(eflr_comp_attr_has_repcode(&dlis->parse_state)){
		avail_bytes = dlis->max_byte_idx - current_byte_idx;
		if (avail_bytes < 1) return -1;
		parse_ushort(&p_buffer[current_byte_idx], (unsigned int *)repcode);
		// parse repcode success
		current_byte_idx += 1; 
	}
	else {
		//*repcode = -1;
	}
	if(eflr_comp_attr_has_unit(&dlis->parse_state)){
		avail_bytes = dlis->max_byte_idx - current_byte_idx;
		if (avail_bytes < 1) return -1;
		int unit_len = parse_ident(&p_buffer[current_byte_idx], dlis->max_byte_idx - current_byte_idx, unit);
		if(unit_len <= 0) {
			//printf("not enough data to parse component units");
			return -1;
		}
		// parse unit success
		current_byte_idx += unit_len;
	}
	else {
		//unit->len = 0;
		//unit->buff = 0;
	}
	/* -- 
        We do not read value here. We will read it outside of this function.
        We do not support the case where default value is specified in template object
       --
    if(eflr_comp_attr_has_value(&dlis->parse_state)){
		avail_bytes = dlis->max_byte_idx - current_byte_idx;
		if (avail_bytes < 1) return -1;
		int nbytes_read = parse_value(&p_buffer[current_byte_idx], avail_bytes, *repcode, val);
        if(nbytes_read < 0) return -1;
		current_byte_idx += nbytes_read;
	}*/

	if (dlis->parse_state.templt_read_idx < 0) {
		eflr_set_template_object_queue(dlis, label, *count, *repcode, unit);
	}
	else {
		eflr_set_template_object_dequeue(dlis);
	}

	return current_byte_idx - dlis->byte_idx;
}
int parse_eflr_component_attrib_value(dlis_t *dlis, value_t *val) {
	int avail_bytes = 0;
    int current_byte_idx = dlis->byte_idx;
    int repcode = dlis->parse_state.attrib_repcode;

	byte_t *p_buffer = dlis->buffer[dlis->buffer_idx];

	if(!eflr_comp_attr_has_value(&dlis->parse_state)) {
        fprintf(stderr, "Unexpected\n"); fflush(stderr);
        exit(-1);
    }

    avail_bytes = dlis->max_byte_idx - current_byte_idx;
    if (avail_bytes < 1) return -1;
    int nbytes_read = parse_value(&p_buffer[current_byte_idx], avail_bytes, repcode, val);
    if (nbytes_read < 0) return -1;
    return nbytes_read;
}
int parse_eflr_component_invatr(dlis_t *dlis) {
    return 0;
}
int parse_eflr_component_object(dlis_t *dlis, obname_t* obname) {
    if(!eflr_comp_object_has_name(&dlis->parse_state)){
        fprintf(stderr, "The component object does not has name");
        exit(-1);
    } 
    byte_t *p_buffer = dlis->buffer[dlis->buffer_idx];
    int current_byte_idx = dlis->byte_idx;

    int obname_len = parse_obname(&p_buffer[current_byte_idx], dlis->max_byte_idx - current_byte_idx, obname);
    if(obname_len <= 0) return -1;
    //parse object success

    return obname_len;
}

int parse_iflr_data(dlis_t *dlis) {
    parse_state_t *state = &dlis->parse_state;
    int lrs_trail_len = 0;
    if(lrs_attr_has_checksum(state)) 
        lrs_trail_len += 2;
    if(lrs_attr_has_trailing(state)) 
        lrs_trail_len += 2;
    if(lrs_attr_has_padding(state)) 
        lrs_trail_len ++;

    if(dlis->max_byte_idx - dlis->byte_idx < state->lrs_len - state->lrs_byte_cnt - lrs_trail_len)
        return dlis->max_byte_idx - dlis->byte_idx;

    return state->lrs_len - state->lrs_byte_cnt - lrs_trail_len;
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
	//printf("--next_state(): vr_len %d, vr_byte_cnt %d, lrs_len %d, lrs_byte_cnt %d\n", 
	//	dlis->parse_state.vr_len, dlis->parse_state.vr_byte_cnt, dlis->parse_state.lrs_len, dlis->parse_state.lrs_byte_cnt);
    int lrs_trail_len = 0;
    if(lrs_attr_has_checksum(&dlis->parse_state)) 
        lrs_trail_len += 2;
    if(lrs_attr_has_trailing(&dlis->parse_state)) 
        lrs_trail_len += 2;
    if(lrs_attr_has_padding(&dlis->parse_state)) 
        lrs_trail_len ++;

	switch(dlis->parse_state.code){
		case EXPECTING_SUL:
        	dlis->parse_state.code = EXPECTING_VR;
			break;
		case EXPECTING_VR:
			dlis->parse_state.code = EXPECTING_LRS;
			break;
		case EXPECTING_LRS:
			if (lrs_attr_is_eflr(&dlis->parse_state)) {
                if (dlis->parse_state.attrib_value_cnt <= 0){ 
                    dlis->parse_state.code = EXPECTING_EFLR_COMP;
                }else {
                    dlis->parse_state.code = EXPECTING_EFLR_COMP_ATTRIB_VALUE;
                }
			}
			else {
                dlis->parse_state.code = EXPECTING_IFLR_DATA;
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
		case EXPECTING_EFLR_COMP_ABSATR:
		case EXPECTING_EFLR_COMP_OBJECT:
			if (dlis->parse_state.lrs_len - dlis->parse_state.lrs_byte_cnt > lrs_trail_len) {
				dlis->parse_state.code = EXPECTING_EFLR_COMP;
			}
            else if (dlis->parse_state.lrs_len - dlis->parse_state.lrs_byte_cnt <= lrs_trail_len) {
                dlis->parse_state.code = EXPECTING_LRS_TRAILING;
            }
            break;
		case EXPECTING_EFLR_COMP_INVATR:
		case EXPECTING_EFLR_COMP_ATTRIB:
            if (eflr_comp_attr_has_value(&dlis->parse_state)) {
                dlis->parse_state.code = EXPECTING_EFLR_COMP_ATTRIB_VALUE;
            }
            else {
                if (dlis->parse_state.lrs_len - dlis->parse_state.lrs_byte_cnt > lrs_trail_len) {
                    dlis->parse_state.code = EXPECTING_EFLR_COMP;
                }
                else if (dlis->parse_state.lrs_len - dlis->parse_state.lrs_byte_cnt <= lrs_trail_len) {
                    dlis->parse_state.code = EXPECTING_LRS_TRAILING;
                }
            }
            break;
        case EXPECTING_LRS_TRAILING: 
			if (dlis->parse_state.vr_len > dlis->parse_state.vr_byte_cnt){
				dlis->parse_state.code = EXPECTING_LRS;
			} 
            else {
				dlis->parse_state.code = EXPECTING_VR;
			}
			break;
        case EXPECTING_EFLR_COMP_ATTRIB_VALUE:
            if(dlis->parse_state.lrs_len - dlis->parse_state.lrs_byte_cnt <= lrs_trail_len){
                dlis->parse_state.code = EXPECTING_LRS_TRAILING;
            } 
            else if (dlis->parse_state.attrib_value_cnt <= 0){  
                    dlis->parse_state.code = EXPECTING_EFLR_COMP;
            }
            break;
        case EXPECTING_IFLR_DATA:
            if(dlis->parse_state.lrs_len - dlis->parse_state.lrs_byte_cnt <= lrs_trail_len) {
                dlis->parse_state.code = EXPECTING_LRS_TRAILING;
            };
            //if(dlis->parse_state.vr_len > dlis->parse_state.vr_byte_cnt){
            //    dlis->parse_state.code = EXPECTING_LRS;
            //} else {
            //    dlis->parse_state.code = EXPECTING_VR;
            //}
            //dlis->parse_state.code = EXPECTING_IFLR_DATA;
            break;
	}
}

void eflr_set_template_object_queue(dlis_t* dlis, sized_str_t *label, long count, int repcode, sized_str_t *unit) {
	//init eflr_template_object_t
	int write_idx = dlis->parse_state.templt_write_idx;
	if (write_idx < 0 || write_idx >= MAX_TEMPLT_OBJS) {
		fprintf(stderr, "There are too many template objects\n");
		exit(-1);
	}
	eflr_template_object_t *templt = dlis->parse_state.templt;
	templt[write_idx].label = *label;
	if (repcode < 0) repcode = DLIS_IDENT;
	templt[write_idx].repcode = repcode;
	templt[write_idx].count = (count < 0)?1:count;
	templt[write_idx].unit = *unit;
	//memcpy(&templt[write_idx].default_value, default_val, sizeof(value_t));

	dlis->parse_state.templt_write_idx++;
    //printf("********queue template object: %d %d\n", dlis->parse_state.templt_write_idx, dlis->parse_state.templt_read_idx);
}
void eflr_set_template_object_get(dlis_t* dlis, sized_str_t *label, long *count, int *repcode, sized_str_t *unit) {
	int read_idx = dlis->parse_state.templt_read_idx;
	if (read_idx < 0 || read_idx >= MAX_TEMPLT_OBJS) {
		fprintf(stderr, "There are too many template objects\n");
		exit(-1);
	}
	if (read_idx >= dlis->parse_state.templt_write_idx) {
		fprintf(stderr, "Index out of bound: %d - %d\n", read_idx, dlis->parse_state.templt_write_idx);
        fflush(stderr);
		exit(-1);
	}
	eflr_template_object_t *templt = dlis->parse_state.templt;
	*label = templt[read_idx].label;
	*repcode = templt[read_idx].repcode;
	*count = templt[read_idx].count;
	*unit = templt[read_idx].unit;
    //printf("******** get template object: %d %d\n", dlis->parse_state.templt_write_idx, dlis->parse_state.templt_read_idx);
	//``memcpy(default_val, &templt[read_idx].default_value, sizeof(value_t));
}

void eflr_set_template_object_dequeue(dlis_t* dlis) {
	dlis->parse_state.templt_read_idx++;
    //printf("******** dequeue get template object: %d %d\n", dlis->parse_state.templt_write_idx, dlis->parse_state.templt_read_idx);
}

void parse(dlis_t *dlis) {
    // vr related variables
    uint32_t vr_len;
	uint32_t vr_version;
    // lrs related variables
    uint32_t lrs_len, lrs_type;
    byte_t lrs_attr;
    byte_t eflr_comp_first_byte;

	//component related variables
	sized_str_t comp_set_type;
	sized_str_t comp_set_name;
	obname_t obname;

    int len;
	sized_str_t label, unit;
	long count;
	int repcode;
	value_t val;
    
	if (dlis->byte_idx == dlis->max_byte_idx) return;

    while (1) {
        //printf("parse loop: parse_state.code:%s, max_byte_idx:%d, byte_idx:%d\n", 
        //    PARSE_STATE_NAMES[dlis->parse_state.code], dlis->max_byte_idx, dlis->byte_idx);

        switch(dlis->parse_state.code) {
            case EXPECTING_SUL:
                len = parse_sul(dlis);
                if (len < 0) goto end_loop;
                // update state
                dlis->byte_idx += len;
				next_state(dlis);
                break;
            case EXPECTING_VR:
                len = parse_vr_header(dlis, &vr_len, &vr_version);
                if(len < 0) goto end_loop;
				//callback
    			dlis->on_visible_record_header_f(dlis->vr_idx, vr_len, vr_version);

                // update state
				dlis->vr_idx++;
                dlis->byte_idx += len;
                dlis->parse_state.vr_len = vr_len;
                dlis->parse_state.vr_byte_cnt = len;
                dlis->parse_state.lrs_len = 0;
                dlis->parse_state.lrs_byte_cnt = 0;
				next_state(dlis);
                break;
            case EXPECTING_LRS:
                len = parse_lrs_header(dlis, &lrs_len, &lrs_attr, &lrs_type);
                if(len < 0) goto end_loop;

                // callback
                if (lrs_attr_is_first_lrs(&dlis->parse_state)) {
                    dlis->on_logical_record_begin_f(dlis->lr_idx, lrs_len, lrs_attr, lrs_type);
                    dlis->lr_idx++;
                }

                // update state
                dlis->lrs_idx++;
                dlis->byte_idx += len;
                dlis->parse_state.lrs_len = lrs_len;
                //dlis->parse_state.lrs_byte_cnt = 0;
                dlis->parse_state.lrs_byte_cnt = len;
                dlis->parse_state.vr_byte_cnt += len;
                dlis->parse_state.lrs_attr = lrs_attr;
                dlis->parse_state.lrs_type = lrs_type;
                //dlis->parse_state.vr_byte_cnt += lrs_len;
				next_state(dlis);
                break;
            case EXPECTING_EFLR_COMP:
                len = parse_eflr_component(dlis, &eflr_comp_first_byte); // parse first byte of eflr component
                if (len < 0) goto end_loop;

                // update state
                dlis->parse_state.eflr_comp_first_byte = eflr_comp_first_byte;
                dlis->byte_idx += len;
                dlis->parse_state.lrs_byte_cnt += len;
                dlis->parse_state.vr_byte_cnt += len;
				next_state(dlis);
                break;
            case EXPECTING_EFLR_COMP_SET:
            case EXPECTING_EFLR_COMP_RSET:
            case EXPECTING_EFLR_COMP_RDSET:
                obname_invalidate(&obname);
                len =  parse_eflr_component_set(dlis, &comp_set_type, &comp_set_name);
                if(len < 0) goto end_loop;
				// initialize dlis.templt. WAITING FOR TEMPLATE OBJECT
				dlis->parse_state.templt_write_idx = 0;
				dlis->parse_state.templt_read_idx = -1;
				// callback
				dlis->on_eflr_component_set_f(&comp_set_type, &comp_set_name);
                // update state
                dlis->byte_idx += len;
                dlis->parse_state.lrs_byte_cnt += len;
                dlis->parse_state.vr_byte_cnt += len;
				next_state(dlis);
                break;
            //case EXPECTING_EFLR_COMP_ABSATR:
			//	next_state(dlis);
            //    break;
            case EXPECTING_EFLR_COMP_ABSATR:
            case EXPECTING_EFLR_COMP_ATTRIB:
            case EXPECTING_EFLR_COMP_INVATR:
				label.len = 0;
				unit.len = 0;
				count = 1;
				repcode = DLIS_IDENT;
                dlis->parse_state.attrib_value_cnt = 0;
                len = parse_eflr_component_attrib(dlis, &label, &count, &repcode, &unit);
                if(len < 0) goto end_loop;

				// callback
				dlis->on_eflr_component_attrib_f(&label, count, repcod, &unit, &obname,
                        eflr_comp_attr_has_value(&dlis->parse_state));

                // update state
                dlis->byte_idx += len;
                dlis->parse_state.lrs_byte_cnt += len;
                dlis->parse_state.vr_byte_cnt += len;
                if (eflr_comp_attr_has_value(&dlis->parse_state)) {
                    dlis->parse_state.attrib_value_cnt = count;
                    dlis->parse_state.attrib_repcode = repcode;
                }
				next_state(dlis);
                break;
            case EXPECTING_EFLR_COMP_ATTRIB_VALUE:
                value_invalidate(&val);
                len = parse_eflr_component_attrib_value(dlis, &val);
                if (len < 0) goto end_loop;

                // callback
                dlis->on_eflr_component_attrib_value_f(dlis->parse_state.attrib_repcode, &val);
                // update state
                dlis->byte_idx += len;
                dlis->parse_state.lrs_byte_cnt += len;
                dlis->parse_state.vr_byte_cnt += len;
                dlis->parse_state.attrib_value_cnt--;
                next_state(dlis);
                break;
            case EXPECTING_EFLR_COMP_OBJECT:
                len = parse_eflr_component_object(dlis, &obname);
                if(len < 0) goto end_loop;
				//start to read object's attributes
				dlis->parse_state.templt_read_idx = 0;

				//callback
				dlis->on_eflr_component_object_f(obname);
                // update status
                dlis->byte_idx += len;
                dlis->parse_state.lrs_byte_cnt += len;
                dlis->parse_state.vr_byte_cnt += len;
				next_state(dlis);
                break;
            case EXPECTING_LRS_TRAILING:
                len = parse_lrs_trailing(dlis);
                if (len < 0) goto end_loop;

                // callback 
                dlis->on_logical_record_end_f(dlis->lrs_idx - 1);

                // update status
                dlis->byte_idx += len;
                dlis->parse_state.lrs_byte_cnt += len;
                dlis->parse_state.vr_byte_cnt += len;
                next_state(dlis);
                break;
            case EXPECTING_IFLR_DATA:
                len = parse_iflr_data(dlis);
                if (len <= 0) goto end_loop;
                // callback 
                dlis->on_iflr_data_f(len);

                // update status
                dlis->byte_idx += len;
                dlis->parse_state.lrs_byte_cnt += len;
                dlis->parse_state.vr_byte_cnt += len;
                next_state(dlis);
                break;
        }
        //usleep(500*1000);
    }
end_loop:
    //usleep(500*1000);
    return;
}

binn *g_obj;
void on_sul(int seq, char *version, char *structure, int max_rec_len, char *ssi) {
    
    g_obj = binn_object();
    binn_object_set_int32(g_obj, (char *)"seq", seq);
    binn_object_set_str(g_obj, (char *)"version", version);
    binn_object_set_str(g_obj, (char *)"structure", structure);
    binn_object_set_int32(g_obj, (char *)"max_rec_len", max_rec_len);
    binn_object_set_str(g_obj, (char *)"ssi", ssi);

    jscall(_eflr_data_, (char *)binn_ptr(g_obj), binn_size(g_obj));
    binn_free(g_obj);
}

void on_visible_record_header(int vr_idx, int vr_len, int version) {
}

void on_logical_record_begin(int lrs_idx, int lrs_len, byte_t lrs_attr, int lrs_type) {
}

void on_logical_record_end(int lrs_idx) {
}

void on_eflr_component_set(sized_str_t *type, sized_str_t *name) {
    g_obj = binn_object();
    binn_object_set_int32(g_obj, (char*)"sending_data_type", _SET);
    serialize_sized_str(g_obj,(char*) "type", type);
    serialize_sized_str(g_obj,(char*) "name", name);
    jscall(_eflr_data_, (char *)binn_ptr(g_obj), binn_size(g_obj));
    binn_free(g_obj);
}

void on_eflr_component_object(obname_t obname){
    g_obj = binn_object();
    binn_object_set_int32(g_obj, (char*)"sending_data_type", _OBJECT);
    binn_object_set_int32(g_obj, (char*)"origin", obname.origin);
    binn_object_set_int32(g_obj, (char*)"copy_number", obname.copy_number);
    serialize_sized_str(g_obj, (char*)"name", &obname.name);
    jscall(_eflr_data_, (char*)binn_ptr(g_obj), binn_size(g_obj));
    binn_free(g_obj);
}

void on_eflr_component_attrib(sized_str_t *label, long count, int repcode, sized_str_t *unit, obname_t *obname, int has_value) {
    g_obj = binn_object();
    serialize_sized_str(g_obj, (char*)"label", label);
    binn_object_set_int32(g_obj, (char*)"count", count);
    binn_object_set_int32(g_obj, (char*)"repcode", repcode);
    serialize_sized_str(g_obj, (char*)"unit", unit);
    if(obname->name.len < 0){
        binn_object_set_int32(g_obj, (char*)"sending_data_type", _OBJ_TEMPLATE);
        jscall(_eflr_data_,(char*)binn_ptr(g_obj), binn_size(g_obj));
    }
    binn_free(g_obj);
}

void on_eflr_component_attrib_value(int repcode, value_t *val) {
    g_obj = binn_object();
    binn_object_set_int32(g_obj, (char*)"sending_data_type", _OBJ_VALUE);

    switch(val->repcode) {
        case DLIS_FSHORT:
        case DLIS_FSINGL:
        case DLIS_FSING1:
        case DLIS_FSING2:
        case DLIS_ISINGL:
        case DLIS_VSINGL:
        case DLIS_FDOUBL:
        case DLIS_FDOUB1:
        case DLIS_FDOUB2:
        case DLIS_CSINGL:
        case DLIS_CDOUBL:
            binn_object_set_double(g_obj, (char*)"values", val->u.double_val);
            break;
        case DLIS_SSHORT:
        case DLIS_SNORM:
        case DLIS_SLONG:
            binn_object_set_int32(g_obj, (char*)"values", val->u.int_val);
            break;
        case DLIS_USHORT:
        case DLIS_UNORM:
        case DLIS_ULONG:
        case DLIS_UVARI:
            binn_object_set_uint32(g_obj, (char*)"values", val->u.uint_val);
            break;
        case DLIS_ORIGIN:
        case DLIS_IDENT:
        case DLIS_UNITS:
        case DLIS_ASCII:
            serialize_sized_str(g_obj, (char*)"values", &val->u.lstr);
            break;
        case DLIS_DTIME:{
                char dtime[100];
                sprintf(dtime,"%d-%02d-%02d %02d:%02d:%02d.%d (%d)",
                        val->u.dtime_val.year + 1900, (val->u.dtime_val.tz_and_month & 0x0F), val->u.dtime_val.day,
                        val->u.dtime_val.hour, val->u.dtime_val.minute, 
                        val->u.dtime_val.second, htons(val->u.dtime_val.ms),
                        (val->u.dtime_val.tz_and_month >> 4));
                binn_object_set_str(g_obj, (char*)"values", dtime);
            }
            break;
        case DLIS_OBNAME:{
                binn* obname_tmp = binn_object();
                binn_object_set_int32(obname_tmp, (char*)"origin", val->u.obname_val.origin);
                binn_object_set_int32(obname_tmp, (char*)"copy_number", val->u.obname_val.copy_number);
                serialize_sized_str(obname_tmp, (char*)"name", &val->u.obname_val.name);
                binn_object_set_object(g_obj, (char*)"values", obname_tmp);
                binn_free(obname_tmp);
            }
            break;
        case DLIS_OBJREF:{
                binn* name_tmp = binn_object();
                binn* tmp = binn_object();
                serialize_sized_str(tmp, (char*)"type", &val->u.objref_val.type);
                binn_object_set_int32(name_tmp, (char*)"origin", val->u.objref_val.name.origin);
                binn_object_set_int32(name_tmp, (char*)"copy_number", val->u.objref_val.name.copy_number);
                serialize_sized_str(name_tmp, (char*)"name", &val->u.objref_val.name.name);
                binn_object_set_object(tmp, (char*)"name", name_tmp);
                binn_object_set_object(g_obj, (char*)"values", tmp);
                binn_free(name_tmp);
                binn_free(tmp);
            }
            break;
        case DLIS_ATTREF:
        case DLIS_STATUS:
            fprintf(stderr, "haven't implemented yet %d\n", val->repcode);
            exit(-1);
            break;
    }    
    jscall(_eflr_data_,(char*)binn_ptr(g_obj), binn_size(g_obj));
    binn_free(g_obj);
}

void on_iflr_data(int data_len) {
}

void do_parse(char *file_name) {
    printf("do_parse %s\n", file_name);
    byte_t buffer[4 * 1024];
    int byte_read;
    dlis_t dlis;
    dlis_init(&dlis);
    dlis.on_sul_f = &on_sul;
    dlis.on_visible_record_header_f = &on_visible_record_header;
    dlis.on_logical_record_begin_f = &on_logical_record_begin;
    dlis.on_logical_record_end_f = &on_logical_record_end;
	dlis.on_eflr_component_set_f = &on_eflr_component_set;
	dlis.on_eflr_component_object_f = &on_eflr_component_object;
	dlis.on_eflr_component_attrib_f = &on_eflr_component_attrib;
    dlis.on_eflr_component_attrib_value_f = &on_eflr_component_attrib_value;
    dlis.on_iflr_data_f = &on_iflr_data;
    FILE *f = fopen(file_name, "rb");
    if (f == NULL) {
        fprintf(stderr, "Error open file %s\n", strerror(errno));
        exit(-1);
    }
    while(!feof(f) ) {
        byte_read = fread(buffer, 1, 100, f);
        if (byte_read < 0) {
            fprintf(stderr,"Error reading file");
            exit(-1);
        }
        dlis_read(&dlis, buffer, byte_read);
    }
    printf("Finish reading file\n");
}
