#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"

int REPCODE_SIZES[] = {
    -1, // Not used
    2,  // DLIS_FSHORT_LEN
    4,  // DLIS_FSINGL_LEN  4
    8,  // DLIS_FSING1_LEN  8
    12, // DLIS_FSING2_LEN  12
    4,  // DLIS_ISINGL_LEN  4
    4,  // DLIS_VSINGL_LEN  4
    8,  // DLIS_FDOUBL_LEN  8
    16, // DLIS_FDOUB1_LEN  16
    24, // DLIS_FDOUB2_LEN  24
    8,  // DLIS_CSINGL_LEN  8
    16, // DLIS_CDOUBL_LEN  16
    1,  // DLIS_SSHORT_LEN  1
    2,  // DLIS_SNORM_LEN   2
    4,  // DLIS_SLONG_LEN   4
    1,  // DLIS_USHORT_LEN  1
    2,  // DLIS_UNORM_LEN   2
    4,  // DLIS_ULONG_LEN   4
    0,  // DLIS_UVARI_LEN   0     //1, 2 or 4
    0,  // DLIS_IDENT_LEN   0    //V
    0,  // DLIS_ASCII_LEN   0    //V
    8,  // DLIS_DTIME_LEN   8
    0,  // DLIS_ORIGIN_LEN  0    //V
    0,  // DLIS_OBNAME_LEN  0    //V
    0,  // DLIS_OBJREF_LEN  0    //V
    0,  // DLIS_ATTREF_LEN  0    //V
    1,  // DLIS_STATUS_LEN  1
    0   // DLIS_UNITS_LEN   0    //V
};


byte_t *locate_free(value_t *value, int size) {
    return (value->u.raw + size);
}
void pack_lstr(sized_str_t *lstr, value_t *v) {
	v->u.lstr.len = lstr->len;
	if (lstr->len > (int) (MAX_VALUE_SIZE - sizeof(sized_str_t))  ) {
		fprintf(stderr, "sized string is too large for packing\n");
		exit(-1);
	}
	v->repcode = DLIS_IDENT;
    v->u.lstr.buff = locate_free(v, sizeof(sized_str_t));
	memcpy(v->u.lstr.buff, lstr->buff, lstr->len);
}
void unpack_lstr(value_t *v, sized_str_t *lstr) {
	lstr->len = v->u.lstr.len;
    if(lstr->buff == NULL) {
        fprintf(stderr, "==> unpack_lstr destination buff null\n");
        exit(-1);
    }
    memmove(lstr->buff, v->u.lstr.buff, lstr->len);
	//lstr->buff = v->u.lstr.buff;
}

void pack_obname(obname_t *obname, value_t *v) {
    v->u.obname_val.origin = obname->origin;
    v->u.obname_val.copy_number = obname->copy_number;
    
    if (obname->name.len > (int)(MAX_VALUE_SIZE - sizeof(obname_t)) ) {
        fprintf(stderr, "obname is too large for packing\n");
        exit(-1);
    }

    v->u.obname_val.name.len = obname->name.len;
    v->u.obname_val.name.buff = locate_free(v, sizeof(obname_t));
    memcpy(v->u.obname_val.name.buff, obname->name.buff, obname->name.len);
}
void unpack_obname(value_t *v, obname_t *obname) {
    obname->origin = v->u.obname_val.origin;
    obname->copy_number = v->u.obname_val.copy_number;
    obname->name = v->u.obname_val.name;
}

void pack_objref(objref_t *objref, value_t *v){
	if (objref->type.len > (int)(MAX_VALUE_SIZE - sizeof(sized_str_t))) {
		fprintf(stderr, "objref is too large for packing\n");
		exit(-1);
	}
    v->repcode = DLIS_OBJREF;
    v->u.objref_val.type.len = objref->type.len;
    v->u.objref_val.type.buff = locate_free(v, sizeof(sized_str_t));
    memmove(v->u.objref_val.type.buff, objref->type.buff, objref->type.len);
    
	if (objref->name.name.len > (int)(MAX_VALUE_SIZE - sizeof(sized_str_t) - sizeof(obname_t) - objref->type.len)) {
		fprintf(stderr, "objref is too large for packing\n");
		exit(-1);
	}

    v->u.objref_val.name.origin = objref->name.origin;
    v->u.objref_val.name.copy_number = objref->name.copy_number;
    v->u.objref_val.name.name.len = objref->name.name.len;
    v->u.objref_val.name.name.buff = locate_free(v, sizeof(sized_str_t) + sizeof(obname_t) + objref->type.len);
    memmove(v->u.objref_val.name.name.buff, objref->name.name.buff, objref->name.name.len);
}

int parse_ushort(byte_t *data, unsigned int *out) {
	if(data == NULL || out == NULL) {
		fprintf(stderr, "Unexpected NULL");
		return -1;
	}
    uint8_t *pval = (uint8_t *)data;
	*out = *pval;
    return 1;
}
int parse_unorm(byte_t *data, unsigned int *out) {
	if(data == NULL || out == NULL) {
		fprintf(stderr, "Unexpected NULL");
		return -1;
	}
    uint16_t *p_tmp = (uint16_t *)data;
	*out = htons(*p_tmp);
    return 2;
}
int parse_ulong(byte_t * buff, unsigned int *out) {
	if(buff == NULL || out == NULL) {
		fprintf(stderr, "Unexpected NULL");
		return -1;
	}
    unsigned int *p_tmp = (unsigned int *)buff;
	*out = htonl(*p_tmp);
    return 4; 
}


int parse_sshort(byte_t *data, int *out) {
	if(data == NULL || out == NULL) {
		fprintf(stderr, "Unexpected NULL");
		return -1;
	}
    int8_t *pval = (int8_t *)data;
	*out = (int) *pval;
    return 1;
}

int parse_snorm(byte_t *data, int *out){
	if(data == NULL || out == NULL) {
		fprintf(stderr, "Unexpected NULL");
		return -1;
	}
    uint16_t uval = *((uint16_t *)data);
    uval = htons(uval);
    int16_t val = (int16_t)uval;
    *out = (int) val;
	return 2;
}

int parse_slong(byte_t *data, int *out){
	if(data == NULL || out == NULL) {
		fprintf(stderr, "Unexpected NULL");
		return -1;
	}
    uint32_t uval = *((uint32_t*)data);
    uval = htonl(uval);
    int32_t val = (int32_t)uval;
    *out = (int) val;
	return 4;
}

int parse_fsingl(byte_t *data, double *out) {
	if(data == NULL || out == NULL) {
		fprintf(stderr, "Unexpected NULL");
		return -1;
	}
    union fsingl {
        float float_val;
        uint32_t long_val;
    } u;
    uint32_t *tmp = (uint32_t*)data;
    u.long_val = htonl(*tmp);
    *out = u.float_val;
	return 4;
}

int parse_fdoubl(byte_t *data, double *out){
	if(data == NULL || out == NULL) {
		fprintf(stderr, "Unexpected NULL");
		return -1;
	}
    union {
        double double_val;
        uint64_t long_val;
    } u;
    u.double_val = 0.0;

    uint32_t *pdata = (uint32_t *)data;

    uint32_t *first = pdata;
    uint32_t *second = pdata + 1;
    
    uint64_t temp = htonl(*first);

    u.long_val = (temp << 32) + htonl(*second);
    *out = u.double_val;
	return 8;
}

size_t trim(char *out, size_t len, const char *str){
    size_t length = strlen(str);
    length = (len > length)?length: len;
    if(length == 0) return 0;

    const char *end;
    size_t out_size;

    // Trim leading space
    while(isspace((unsigned char)*str) && length > 0) {
        length--;
        str++;
    }

    if (length == 0) { // All spaces?
        *out = 0;
        return 1;
    }

    if(*str == 0) { // All spaces?
        *out = 0;
        return 1;
    }

    // Trim trailing space
    end = str + length - 1;
    while(end > str && isspace((unsigned char)*end)) {
        end--;
    }
    end++;

    // Set output size to minimum of trimmed string length and buffer size minus 1
    out_size = (end - str);

    // Copy trimmed string and add null terminator
    memcpy(out, str, out_size);
    out[out_size] = 0;

    return out_size;
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

/* check if str is a number (return 1). If it is not, return 0 */
int is_integer(char *str, int len) {
    while (len > 0 && *str != 0 )  {
        if(!isdigit(*str)) {
            return 0;
        }
        str++;
        len--;
    }
    return 1;
}

int parse_ident(byte_t *buff, int buff_len, sized_str_t *output) {
    if (output == NULL || buff == NULL || buff_len <= 1) {
        return -1;
    }
    unsigned int len = 0;
	parse_ushort(buff, &len);
    if ((buff_len-1) < (int)len) return -1; // Not enough data to parse
    output->buff = (buff + 1);
    output->len = len;
    //printf("... value: %.*s\n", len, output->buff);
    return len + 1;
}

int parse_ascii(byte_t *buff, int buff_len, sized_str_t *output){ 
    if (output == NULL || buff == NULL || buff_len <= 1) {
        return -1;
    }
    unsigned int len = 0;
    int nbytes_read = parse_uvari(buff, buff_len, &len);
    if(nbytes_read <= 0 || buff_len - nbytes_read < (int)len) return -1;
    output->buff = (buff + nbytes_read);
    output->len = len;
    return len + nbytes_read;
}

int parse_uvari(byte_t * buff, int buff_len, unsigned int* output){
    int len = 0;
    if(output == NULL || buff == NULL || buff_len <= 0){
        return -1;
    }
    if(!((*buff) & 0x80)) {
        parse_ushort(buff, output);
        len = 1;
    }
    else if(!((*buff) & 0x40)) {
        if(buff_len < 2) return -1;
		uint32_t tmp = 0;
        parse_unorm(buff, &tmp);
		*output = tmp - 0x8000;
        len = 2;
    }
    else {
        if(buff_len < 4) return -1;
		uint32_t tmp = 0;
		parse_ulong(buff, &tmp); 
        *output = tmp - 0xC0000000;
        len = 4;
    }
    return len;
}

int parse_value(byte_t* buff, int buff_len, int repcode, value_t *output){
    //printf("--> parse value: repcode %d buff_len %d\n", repcode, buff_len);
    sized_str_t str;
    obname_t obname;
    objref_t objref;
    int repcode_len = 0;

    if (repcode >= DLIS_REPCODE_MAX) {
        fprintf(stderr, "encounter wrong repcode: %d\n", repcode);
        exit(-1);
    }
    repcode_len = REPCODE_SIZES[repcode];
	
	output->repcode = repcode;
    if (buff_len <= 0 || buff_len < repcode_len) {
        //printf("--- parse_value buffer is not enough ---\n");
        return -1;
    }
    switch(repcode) {
        case DLIS_FSHORT:
            break;
        case DLIS_FSINGL:
            parse_fsingl(buff, &output->u.double_val);
            break;
        case DLIS_FSING1:
            break;
        case DLIS_FSING2:
            break;
        case DLIS_ISINGL:
            break;
        case DLIS_VSINGL:
            break;
        case DLIS_FDOUBL:
            parse_fdoubl(buff, &output->u.double_val);
            break;
        case DLIS_FDOUB1:
            break;
        case DLIS_FDOUB2:
            break;
        case DLIS_CSINGL:
            break;
        case DLIS_CDOUBL:
            break;
        case DLIS_SSHORT:
            parse_sshort(buff, &output->u.int_val);
            break;
        case DLIS_SNORM:
            parse_snorm(buff, &output->u.int_val);
            break;
        case DLIS_SLONG:
            parse_slong(buff, &output->u.int_val);
            break;
        case DLIS_USHORT:
            parse_ushort(buff, &output->u.uint_val);
            break;
        case DLIS_UNORM:
            parse_unorm(buff, &output->u.uint_val);
            break;
        case DLIS_ULONG:
            parse_ulong(buff, &output->u.uint_val);
            break;
        case DLIS_UVARI:
        case DLIS_ORIGIN:
            repcode_len = parse_uvari(buff, buff_len, &output->u.uint_val);
            break;
        case DLIS_IDENT:
        case DLIS_UNITS:
            repcode_len = parse_ident(buff, buff_len, &str);
			if(repcode_len > 0) {
				pack_lstr(&str, output);
			}
            break;
        case DLIS_ASCII:
            repcode_len = parse_ascii(buff, buff_len, &str);
			if(repcode_len > 0) {
				pack_lstr(&str, output);
			}
            break;
        case DLIS_OBNAME:
            repcode_len = parse_obname(buff, buff_len, &obname);
            if (repcode_len > 0) {
                pack_obname(&obname, output);
            }
            break;
        case DLIS_DTIME:
            repcode_len = parse_dtime(buff, buff_len, &output->u.dtime_val);
            break;
        case DLIS_OBJREF:
            repcode_len = parse_objref(buff, buff_len, &objref);
            if(repcode_len > 0){
                pack_objref(&objref, output);
            }
            break;
        case DLIS_ATTREF:
            break;
        case DLIS_STATUS:
            parse_ushort(buff, &output->u.uint_val);
            break;
    }
    return repcode_len;
}

int parse_obname(byte_t *buff, int buff_len, obname_t* obname){
    if(buff == NULL || obname == NULL || buff_len <= 0)  return -1;

    int origin_len = parse_uvari(buff, buff_len, &obname->origin);
    if(origin_len <= 0) return -1;

    parse_ushort(buff + origin_len, &obname->copy_number);

    int name_len = parse_ident(buff + origin_len + 1, buff_len - origin_len - 1, &obname->name);
    if(name_len <= 0) return -1;
    return origin_len + 1 + name_len;
}

int parse_objref(byte_t *buff, int buff_len, objref_t* objref){
    if(buff == NULL || objref == NULL || buff_len <= 0)  return -1;
    int type_len = parse_ident(buff, buff_len, &objref->type);
    if(type_len < 0) return -1;
    int name_len = parse_obname(buff + type_len, buff_len - type_len, &objref->name);
    if(name_len < 0) return -1;
    return type_len + name_len;
}

int parse_dtime(byte_t *buff, int buff_len, dtime_t *dtime) {
    if ( buff_len < (int)sizeof(dtime_t) ) return -1;
    dtime_t *p_dtime = (dtime_t *)buff;
    *dtime = *p_dtime;
    return sizeof(dtime_t);
}

void print_str(sized_str_t *str){
    int len = 0;
    if (str->len >= 0) {
        len = _printf(__g_cbuff, __g_clen, "|%.*s|", str->len, str->buff);
        __g_cend(len);
    }
}
void print_obname(obname_t *obname){
    int len = 0;
    if (obname->name.len > 0) {
        len = _printf(__g_cbuff, __g_clen, "(%d-%d-%.*s)", 
            obname->origin, obname->copy_number, obname->name.len, obname->name.buff);
        __g_cend(len);
    }
}
void print_objref(objref_t *objref){
    int len = _printf(__g_cbuff, __g_clen, "type:");
    __g_cend(len);
    print_str(&objref->type);
    len = _printf(__g_cbuff, __g_clen, "\tname:");
    __g_cend(len);
    print_obname(&objref->name);
}

void print_dtime(dtime_t *dtime) {
    int len = _printf(__g_cbuff, __g_clen,
            "%d-%02d-%02d %02d:%02d:%02d.%d (%d)",
           // "%2d:%2d:%2d.%3d %2d-%2d-%4d (%d)", 
            dtime->year + 1900, (dtime->tz_and_month & 0x0F), dtime->day,
            dtime->hour, dtime->minute, 
            dtime->second, htons(dtime->ms),
            (dtime->tz_and_month >> 4)
    );
    __g_cend(len);
}

void print_value(value_t *val) {
    int len;
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
            len = _printf(__g_cbuff, __g_clen, "value=%f", val->u.double_val);
            __g_cend(len);
            break;
        case DLIS_SSHORT:
        case DLIS_SNORM:
        case DLIS_SLONG:
            len = _printf(__g_cbuff, __g_clen, "value=%d", val->u.int_val);
            __g_cend(len);
            break;
        case DLIS_USHORT:
        case DLIS_UNORM:
        case DLIS_ULONG:
        case DLIS_UVARI:
        case DLIS_STATUS:
            len = _printf(__g_cbuff, __g_clen, "value=%d", val->u.uint_val);
            __g_cend(len);
            break;
        case DLIS_ORIGIN:
        case DLIS_IDENT:
        case DLIS_UNITS:
        case DLIS_ASCII:
            len = _printf(__g_cbuff, __g_clen, "value="); 
            __g_cend(len);
            print_str(&val->u.lstr);
            break;
        case DLIS_DTIME:
            len = _printf(__g_cbuff, __g_clen, "value="); 
            __g_cend(len);
            print_dtime(&val->u.dtime_val);
            break;
        case DLIS_OBNAME:
            len = _printf(__g_cbuff, __g_clen, "value="); 
            __g_cend(len);
            print_obname(&val->u.obname_val);
            break;
        case DLIS_OBJREF:
            len = _printf(__g_cbuff, __g_clen, "value=");
            __g_cend(len);
            print_objref(&val->u.objref_val);
            break;
        case DLIS_ATTREF:
            fprintf(stderr, "haven't implemented yet %d\n", val->repcode);
            exit(-1);
            break;
    }
}

int (*jsprint_f)(int f_idx, char *buff);
int (*send_to_js_f)(int f_idx, char *buff, int len);
int jsprint(int f_idx, char *buff) {
    if (!jsprint_f) {
        fprintf(stderr, "print function pointer is not set\n");
        exit(-1);
    }
    return jsprint_f(f_idx, buff);
}
void serialize_sized_str(binn* obj, char* key, sized_str_t* str){
    //printf("serialize_sized_str: len=%d, str=%.*s\n", str->len, str->len, str->buff);
    char _str[str->len + 1];
    memmove(_str, str->buff, str->len);
    _str[str->len] = '\0';
    binn_object_set_str(obj, key, _str);
}
void serialize_obname(binn* obj, char* key, obname_t* obname) {
    if(key == NULL){
        binn_object_set_int32(obj, (char *)"origin", obname->origin);
        binn_object_set_int32(obj, (char *)"copy_number", obname->copy_number);
        serialize_sized_str(obj, (char *)"name", &obname->name);
    }else {
        binn *inner_obj = binn_object();
        
        binn_object_set_int32(inner_obj, (char *)"origin", obname->origin);
        binn_object_set_int32(inner_obj, (char *)"copy_number", obname->copy_number);
        serialize_sized_str(inner_obj, (char *)"name", &obname->name);

        binn_object_set_object(obj, key, inner_obj);
        binn_free(inner_obj);
    }
}

void serialize_value(binn* g_obj, char* key, value_t* val){
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
            binn_object_set_double(g_obj, key, val->u.double_val);
            break;
        case DLIS_SSHORT:
        case DLIS_SNORM:
        case DLIS_SLONG:
            binn_object_set_int32(g_obj, key, val->u.int_val);
            break;
        case DLIS_USHORT:
        case DLIS_UNORM:
        case DLIS_ULONG:
        case DLIS_UVARI:
        case DLIS_STATUS:
            binn_object_set_uint32(g_obj, key, val->u.uint_val);
            break;
        case DLIS_ORIGIN:
        case DLIS_IDENT:
        case DLIS_UNITS:
        case DLIS_ASCII:
            serialize_sized_str(g_obj, key, &val->u.lstr);
            break;
        case DLIS_DTIME:{
                char dtime[100];
                sprintf(dtime,"%d-%02d-%02d %02d:%02d:%02d.%d (%d)",
                        val->u.dtime_val.year + 1900, (val->u.dtime_val.tz_and_month & 0x0F), val->u.dtime_val.day,
                        val->u.dtime_val.hour, val->u.dtime_val.minute, 
                        val->u.dtime_val.second, htons(val->u.dtime_val.ms),
                        (val->u.dtime_val.tz_and_month >> 4));
                binn_object_set_str(g_obj, key, dtime);
            }
            break;
        case DLIS_OBNAME:
            serialize_obname(g_obj, key, &val->u.obname_val);
            break;
        case DLIS_OBJREF:{
                binn* tmp = binn_object();
                serialize_sized_str(tmp, (char*)"type", &val->u.objref_val.type);
                serialize_obname(tmp, (char *)"name", &val->u.objref_val.name);
                binn_object_set_object(g_obj, key, tmp);
                binn_free(tmp);
            }
            break;
        case DLIS_ATTREF:
            fprintf(stderr, "haven't implemented yet %d\n", val->repcode);
            exit(-1);
            break;
    }    
}
void serialize_list_add(binn* g_obj, value_t* val){
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
            binn_list_add_double(g_obj, val->u.double_val);
            break;
        case DLIS_SSHORT:
        case DLIS_SNORM:
        case DLIS_SLONG:
            binn_list_add_int32(g_obj, val->u.int_val);
            break;
        case DLIS_USHORT:
        case DLIS_UNORM:
        case DLIS_ULONG:
        case DLIS_UVARI:
        case DLIS_STATUS:
            binn_list_add_uint32(g_obj, val->u.uint_val);
            break;
        case DLIS_ORIGIN:
        case DLIS_IDENT:
        case DLIS_UNITS:
        case DLIS_ASCII:{
            char _str[val->u.lstr.len + 1];
            memmove(_str, val->u.lstr.buff, val->u.lstr.len);
            _str[val->u.lstr.len] = '\0';
            binn_list_add_str(g_obj, _str);
        }
            break;
        case DLIS_DTIME:{
                char dtime[100];
                sprintf(dtime,"%d-%02d-%02d %02d:%02d:%02d.%d (%d)",
                        val->u.dtime_val.year + 1900, (val->u.dtime_val.tz_and_month & 0x0F), val->u.dtime_val.day,
                        val->u.dtime_val.hour, val->u.dtime_val.minute, 
                        val->u.dtime_val.second, htons(val->u.dtime_val.ms),
                        (val->u.dtime_val.tz_and_month >> 4));
                binn_list_add_str(g_obj, dtime);
            }
            break;
        case DLIS_OBNAME:{
            binn* tmp = binn_object();
            serialize_obname(tmp, NULL, &val->u.obname_val);
            binn_list_add_object(g_obj, tmp);
            binn_free(tmp);
        }
            break;
        case DLIS_OBJREF:{
                binn* tmp = binn_object();
                serialize_sized_str(tmp, (char*)"type", &val->u.objref_val.type);
                serialize_obname(tmp, (char *)"name", &val->u.objref_val.name);
                binn_list_add_object(g_obj, tmp);
                binn_free(tmp);
            }
            break;
        case DLIS_ATTREF:
            fprintf(stderr, "haven't implemented yet %d\n", val->repcode);
            exit(-1);
            break;
    }    
}

double get_scalar_value(value_t* val){
    double retval = 0;
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
            retval = val->u.double_val;
            break;
        case DLIS_SSHORT:
        case DLIS_SNORM:
        case DLIS_SLONG:
            retval = val->u.int_val;
            break;
        case DLIS_USHORT:
        case DLIS_UNORM:
        case DLIS_ULONG:
        case DLIS_UVARI:
        case DLIS_STATUS:
            retval = val->u.uint_val;
            break;
        default: 
            printf("This is not scalar value!!!\n");
    }
    return retval;
}
