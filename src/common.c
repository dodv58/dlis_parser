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

int parse_ushort(byte_t *data) {
    uint8_t *pval = (uint8_t *)data;
    return (int)*pval;
}
int parse_unorm(byte_t *data) {
    uint16_t *p_tmp = (uint16_t *)data;
    return htons(*p_tmp);
}
unsigned int parse_ulong(byte_t * buff) {
    unsigned int *p_tmp = (unsigned int *)buff;
    return htonl(*p_tmp);
}


int parse_sshort(byte_t *data) {
    int8_t *pval = (int8_t *)data;
    return (int) *pval;
}

int parse_snorm(byte_t *data){
    uint16_t uval = *((uint16_t *)data);
    uval = htons(uval);
    int16_t val = (int16_t)uval;
    return (int) val;
}

int parse_slong(byte_t *data){
    uint32_t uval = *((uint32_t*)data);
    uval = htonl(uval);
    int32_t val = (int32_t)uval;
    return (int) val;
}

size_t trim(char *out, size_t len, const char *str){
    int length = strlen(str);
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
    if (output == NULL || buff == NULL) {
        return -1;
    }
    int len = parse_ushort(buff);
    if ((buff_len-1) < len) return 0; // Not enough data to parse
    output->buff = (buff + 1);
    output->len = len;
    return len;
}

int parse_uvari(byte_t * buff, int buff_len, int* output){
    int len = 0;
    if(output == NULL || buff == NULL){
        return -1;
    }
    if(!((*buff) & 0x80)) {
        *output = parse_ushort(buff);
        len = 1;
    }
    else if(!((*buff) & 0x40)) {
        if(buff_len < 2) return -1;
        *output = parse_unorm(buff) - 0x8000;
        len = 2;
    }
    else {
        if(buff_len < 4) return -1;
        *output = parse_ulong(buff) - 0xC0000000;
        len = 4;
    }
    return len;
}

int parse_value(byte_t* buff, int buff_len, int repcode, void *output){
    unsigned int *uint_val;
    int *int_val;
    int repcode_len = 0;
    if (repcode >= DLIS_REPCODE_MAX) {
        fprintf(stderr, "encounter wrong repcode\n");
        exit(-1);
    }
    repcode_len = REPCODE_SIZES[repcode];
    if (buff_len < repcode_len) return -1;
    switch(repcode) {
        case DLIS_FSHORT:
            break;
        case DLIS_FSINGL:
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
            int_val = (int *)output;
            *int_val = parse_sshort(buff);
            break;
        case DLIS_SNORM:
            int_val = (int*) output;
            *int_val = parse_snorm(buff);
            break;
        case DLIS_SLONG:
            int_val = (int*) output;
            *int_val = parse_slong(buff);
            break;
        case DLIS_USHORT:
            uint_val = (unsigned int *)output;
            *uint_val = parse_ushort(buff);
            break;
        case DLIS_UNORM:
            uint_val = (unsigned int *)output;
            *uint_val = parse_unorm(buff);
            break;
        case DLIS_ULONG:
            uint_val = (unsigned int *)output;
            *uint_val = parse_ulong(buff);
            break;
        case DLIS_UVARI:
            break;
        case DLIS_IDENT:
            break;
        case DLIS_ASCII:
            break;
        case DLIS_DTIME:
            break;
        case DLIS_ORIGIN:
            break;
        case DLIS_OBNAME:
            break;
        case DLIS_OBJREF:
            break;
        case DLIS_ATTREF:
            break;
        case DLIS_STATUS:
            break;
        case DLIS_UNITS:
            break;
    }
    return repcode_len;
}

int parse_values(byte_t *buff, int buff_len, int val_cnt, int repcode) {
    return 0;
}
