#include "utils.h"

void doPrint(char* func_name, char *msg) {
	printf("==>%s(): %s\n",func_name,msg);
}



int utils_read_data(void* dest, void *data, int code){
	int nbytes = 0;
	switch (code){
		case DLIS_FSHORT:	break; 
		case DLIS_FSINGL: 	break; 
		case DLIS_FSING1: 	break; 
		case DLIS_FSING2: 	break; 
		case DLIS_ISINGL: 	break; 
		case DLIS_VSINGL: 	break; 
		case DLIS_FDOUBL: 	break; 
		case DLIS_FDOUB1: 	break; 
		case DLIS_FDOUB2: 	break; 
		case DLIS_CSINGL: 	break; 
		case DLIS_CDOUBL: 	break; 
		case DLIS_SSHORT: 	break; 
		case DLIS_SNORM: 	break; 
		case DLIS_SLONG: 	break; 
		case DLIS_USHORT: 	
			hex_to_number(dest, data, 1);
			break; 
		case DLIS_UNORM: 	break; 
		case DLIS_ULONG: 	break; 
		case DLIS_UVARI: {
			nbytes = utils_read_uvari((unsigned int *)&dest, data);
			break;
		}
		case DLIS_IDENT: 	break; 
		case DLIS_ASCII: 	break; 
		case DLIS_DTIME: 	break; 
		case DLIS_ORIGIN: 	break; 
		case DLIS_OBNAME: {
			nbytes = utils_read_obname((obname_t * )dest, (unsigned char *) data);
			break; 
		}
		case DLIS_OBJREF: 	{
			break; 
		}
		case DLIS_ATTREF: 	break; 
		case DLIS_STATUS: 	break; 
		case DLIS_UNITS: 	break; 

	}
	return nbytes;
}

unsigned int utils_get_length(unsigned int code){
	switch (code){
		case DLIS_FSHORT:	return DLIS_FSHORT_LEN;
		case DLIS_FSINGL: 	return DLIS_FSINGL_LEN;
		case DLIS_FSING1: 	return DLIS_FSING1_LEN;
		case DLIS_FSING2: 	return DLIS_FSING2_LEN;
		case DLIS_ISINGL: 	return DLIS_ISINGL_LEN;
		case DLIS_VSINGL: 	return DLIS_VSINGL_LEN;
		case DLIS_FDOUBL: 	return DLIS_FDOUBL_LEN;
		case DLIS_FDOUB1: 	return DLIS_FDOUB1_LEN;
		case DLIS_FDOUB2: 	return DLIS_FDOUB2_LEN;
		case DLIS_CSINGL: 	return DLIS_CSINGL_LEN;
		case DLIS_CDOUBL: 	return DLIS_CDOUBL_LEN;
		case DLIS_SSHORT: 	return DLIS_SSHORT_LEN;
		case DLIS_SNORM: 	return DLIS_SNORM_LEN;
		case DLIS_SLONG: 	return DLIS_SLONG_LEN;
		case DLIS_USHORT: 	return DLIS_USHORT_LEN;
		case DLIS_UNORM: 	return DLIS_UNORM_LEN;
		case DLIS_ULONG: 	return DLIS_ULONG_LEN;
		case DLIS_UVARI: 	return DLIS_UVARI_LEN;
		case DLIS_IDENT: 	return DLIS_IDENT_LEN;
		case DLIS_ASCII: 	return DLIS_ASCII_LEN;
		case DLIS_DTIME: 	return DLIS_DTIME_LEN;
		case DLIS_ORIGIN: 	return DLIS_ORIGIN_LEN;
		case DLIS_OBNAME: 	return DLIS_OBNAME_LEN;
		case DLIS_OBJREF: 	return DLIS_OBJREF_LEN;
		case DLIS_ATTREF: 	return DLIS_ATTREF_LEN;
		case DLIS_STATUS: 	return DLIS_STATUS_LEN;
		case DLIS_UNITS: 	return DLIS_UNITS_LEN;
	}
	return 0;
}


void hex_to_number(void *dest, void*data, int len) {
    unsigned char bytes[len];
    memmove(bytes, data, len);
    int sum = 0;
    int i = 0;
    while(i < len){
        sum = sum | bytes[i]<<((len-i-1)*8);
        i++;
    }
    *(int*)dest = sum;
}


int utils_read_uvari(unsigned int * dest, void * data){
    int tmp = 0;
    hex_to_number(&tmp, data, 1);
    if(tmp < 128) {
        *dest = tmp;
        return 1;
    }
    else if(tmp < 192) {
        hex_to_number(&tmp, data, 2);
        *dest = tmp - 32768; //1000 0000 0000 0000
        return 2;
    }
    else {
        hex_to_number(&tmp, data, 4);
        *dest = tmp - 2147483648; //1100 0000 0000 0000 0000 0000 0000 0000
        return 4;
    }
}

int utils_read_obname(obname_t * dest, unsigned char * data){
    int len = 0;
    int nbytes = 0;

    len = utils_read_uvari(&dest->origin, data);
    data += len;
    nbytes += len;

    int origin_len = len;
    

    hex_to_number(&dest->copy_number, data, 1);
    data++;

    hex_to_number(&len, data, 1);
    data++;
    nbytes += 2 + len;

    dest->name = malloc(len + 1);

    memset(dest->name, len + 1, '\0');
    memmove(dest->name, data, len);

    // printf("utils_read_obname===> ");
    data -= origin_len + 2;

    // for(int i =0; i < nbytes; i++){
    //     printf("_%x_", data[i]);
    // }
    // printf("\n");
    return nbytes;
}


int utils_read_objref(unsigned char* type, obname_t* obname, unsigned char* data){
    int nbytes_read = 0;
    int len = 0;
    hex_to_number(&len, data, 1);
    data ++;
    type = malloc(len+1);
    memmove(type, data, len);
    type[len] = '\0';
    nbytes_read += len + 1;
    data += len;

    len = utils_read_obname(obname, data);
    nbytes_read += len;
    return nbytes_read;
}


bool str_to_number(void *dest, void * data, int len){
    char * endptr;
    unsigned char temp[len+1];
    memset(temp, '\0', len+1);
    memmove(temp, data, len);
    if(len <= 4) {
        *(int*)dest = strtoimax((char*)temp, &endptr, 10);
        //*(int*)dest = atoi((char*)temp);
        return true;
    } else {
        *(long long*)dest = strtoll((char*)temp, &endptr, 10);
        return true;
    }
}

int utils_read_data_to_str(unsigned char* dest, void* data, int code){
	if(dest == NULL){
		dest = malloc(1 * sizeof(unsigned char));
	}
	int nbytes = 0;
	switch (code){
		case DLIS_FSHORT:	break; 
		case DLIS_FSINGL: 	break; 
		case DLIS_FSING1: 	break; 
		case DLIS_FSING2: 	break; 
		case DLIS_ISINGL: 	break; 
		case DLIS_VSINGL: 	break; 
		case DLIS_FDOUBL: 	break; 
		case DLIS_FDOUB1: 	break; 
		case DLIS_FDOUB2: 	break; 
		case DLIS_CSINGL: 	break; 
		case DLIS_CDOUBL: 	break; 
		case DLIS_SSHORT: 	break; 
		case DLIS_SNORM: 	break; 
		case DLIS_SLONG: 	break; 
		case DLIS_USHORT: {	
			
			hex_to_number(dest, data, 1);
			break; 
		}
		case DLIS_UNORM: 	break; 
		case DLIS_ULONG: 	break; 
		case DLIS_UVARI: {
			nbytes = utils_read_uvari((unsigned int *)&dest, data);
			break;
		}
		case DLIS_IDENT: 	break; 
		case DLIS_ASCII: 	break; 
		case DLIS_DTIME: 	break; 
		case DLIS_ORIGIN: 	break; 
		case DLIS_OBNAME: {
			nbytes = utils_read_obname((obname_t * )dest, (unsigned char *) data);
			break; 
		}
		case DLIS_OBJREF: 	{
			break; 
		}
		case DLIS_ATTREF: 	break; 
		case DLIS_STATUS: 	break; 
		case DLIS_UNITS: 	break; 

	}

	return nbytes;
}

