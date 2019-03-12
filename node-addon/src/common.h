#ifndef __DLIS_COMMON__
#define __DLIS_COMMON__
#include <arpa/inet.h>
#include <binn.h>
#include <stdarg.h>
typedef unsigned char byte_t;

extern char g_buff[1024];
extern int g_idx;

#define __g_cbuff (g_buff + g_idx)
#define __g_clen (1024 - g_idx)
#define __g_cstart(v) g_idx = v
#define __g_cend(len)  g_idx+=len;g_buff[g_idx]=0

//#define _printf  if(0) printf
#define _printf snprintf
#define app_print(f_idx, buff) printf("%s", buff)
//#define app_print(f_idx, buff) jsprint(f_idx, buff)

extern int (*jsprint_f)(int f_idx, char *buff);
extern int (*send_to_js_f)(int f_idx, char *buff, int len);

typedef enum sending_data_type_e{
    _SET = 0,
    _OBJECT = 1,
    _OBJ_VALUE = 2
} sending_type_t;

typedef struct sized_str_s {
    byte_t *buff;
    int len;
} sized_str_t;

typedef struct dtime_s {
    byte_t year;
    byte_t tz_and_month;
    byte_t day;
    byte_t hour;
    byte_t minute;
    byte_t second;
    uint16_t ms;
} dtime_t;

typedef struct obname_s {
    uint32_t origin;
    uint32_t copy_number;
    sized_str_t name;
} obname_t;

typedef struct objref_s {
    sized_str_t type;
    obname_t name;
} objref_t;

#define MAX_VALUE_SIZE 2048 // TO BE FIXED
struct value_s {
	int repcode;
	union {
		int int_val;
		unsigned int uint_val;
		double double_val;
		sized_str_t lstr;
        dtime_t dtime_val;
        obname_t obname_val;
        objref_t objref_val;
		byte_t raw[MAX_VALUE_SIZE];
	} u;
};
typedef struct value_s value_t;

#define value_invalidate(v) (v)->repcode=-1

#define obname_invalidate(obname) (obname)->name.len = -1

enum rep_code_e {
    DLIS_FSHORT = 	1,		//2
    DLIS_FSINGL = 	2,		//4
    DLIS_FSING1 = 	3,		//8
    DLIS_FSING2 = 	4,		//12
    DLIS_ISINGL = 	5,		//4
    DLIS_VSINGL = 	6,		//4
    DLIS_FDOUBL = 	7,		//8
    DLIS_FDOUB1 = 	8,		//16
    DLIS_FDOUB2 = 	9,		//24
    DLIS_CSINGL = 	10,		//8
    DLIS_CDOUBL = 	11,		//16
    DLIS_SSHORT = 	12,		//1
    DLIS_SNORM = 	13,		//2
    DLIS_SLONG = 	14,		//4
    DLIS_USHORT = 	15,		//1
    DLIS_UNORM = 	16,		//2
    DLIS_ULONG = 	17,		//4
    DLIS_UVARI = 	18,		//1, 2 or 4
    DLIS_IDENT = 	19,		//V
    DLIS_ASCII = 	20,		//V
    DLIS_DTIME = 	21,		//8
    DLIS_ORIGIN = 	22,		//V
    DLIS_OBNAME = 	23,		//V
    DLIS_OBJREF = 	24,		//V
    DLIS_ATTREF = 	25,		//V
    DLIS_STATUS = 	26,		//1
    DLIS_UNITS = 	27,		//V
    DLIS_REPCODE_MAX = 28
};
typedef enum rep_code_e rep_code_t;

// For node module addon
typedef enum node_addon_e {
    _eflr_data_ = 0,
    //_iflr_header_ = 1,
    NCALLBACKS = 1
} node_addon_t;


void pack_lstr(sized_str_t *lstr, value_t *v);
void unpack_lstr(value_t *v, sized_str_t *lstr);
void pack_obname(obname_t *obname, value_t *v);
void unpack_obname(value_t *v, obname_t *obname);
void pack_objref(objref_t *objref, value_t *v);

int parse_ushort(byte_t *data, unsigned int *out);
int parse_unorm(byte_t *data, unsigned int *out);
int parse_ulong(byte_t *data, unsigned int *out);
int parse_sshort(byte_t *data, int *out);
int parse_snorm(byte_t *data, int *out);
int parse_slong(byte_t *data, int *out);
int parse_fsingl(byte_t *data, double *out);
int parse_fdoubl(byte_t *data, double *out);
int parse_uvari(byte_t *buff, int buff_len, unsigned int *output); //return nbytes read
int parse_ident(byte_t *buff, int buff_len, sized_str_t *output);
int parse_ascii(byte_t *buff, int buff_len, sized_str_t *output);
int parse_obname(byte_t *buff, int buff_len, obname_t* obname);
int parse_objref(byte_t *buff, int buff_len, objref_t* objref);
int parse_dtime(byte_t *buff, int buff_len, dtime_t *dtime);

int parse_value(byte_t* buff, int buff_len, int rep_code, value_t* output);

size_t trim(char *out, size_t len, const char *str);
void hexDump (char *desc, void *addr, int len);
int is_integer(char *str, int len);
void print_str(sized_str_t *str);
void print_obname(obname_t *obname);
void print_objref(objref_t *objref);
void print_dtime(dtime_t *dtime);
void print_value(value_t *val);

void serialize_sized_str(binn* obj, char* key, sized_str_t* str);
void serialize_obname(binn* obj, char* key, obname_t* obname);
void serialize_value(binn* g_obj, char* key, value_t* val);
void serialize_list_add(binn* g_obj, value_t* val);

int jsprint(int f_idx, char *buff);

double get_scalar_value(value_t* val);
bool is_null_value(binn* val);
#endif
