#include <stdio.h>
#include <ctype.h>
#include <string.h>
int parse_ushort(byte_t *data) {
    return (int)*data;
}
int parse_unorm(byte_t *data) {
    int high = (int)data[0];
    int low = (int)data[1];
    //int result = low + (high<<8);
    return low + (high<<8);
}

size_t trim(char *out, size_t len, const char *str)
{
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

int parse_ident(char *buff, int buff_len, sized_str_t *output) {
    if (output == 0 || buff == 0) {
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
    if(output = 0 || buff == 0){
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

int parse_ulong(byte_t * buff){
    int tmp = 0;
    for(int i = 0; i < 4; i++){
        tmp = tmp | buff[i] << ((3 - i) * 8);
    }
    return tmp;
}
