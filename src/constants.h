#ifndef __CONSTANTS__
#define __CONSTANTS__

#define BUFF_SIZE 	102400

//Definition of Component Role
#define ABSATR 		0
#define ATTRIB		32
#define INVATR 		64
#define OBJECT		96
#define RDSET		160
#define RSET 		192
#define SET			224


//representation code			//BYTES
#define DLIS_FSHORT 	1		//2
#define DLIS_FSINGL 	2		//4
#define DLIS_FSING1 	3		//8
#define DLIS_FSING2 	4		//12
#define DLIS_ISINGL 	5		//4
#define DLIS_VSINGL 	6		//4
#define DLIS_FDOUBL 	7		//8
#define DLIS_FDOUB1 	8		//16
#define DLIS_FDOUB2 	9		//24
#define DLIS_CSINGL 	10		//8
#define DLIS_CDOUBL 	11		//16
#define DLIS_SSHORT 	12		//1
#define DLIS_SNORM 		13		//2
#define DLIS_SLONG 		14		//4
#define DLIS_USHORT 	15		//1
#define DLIS_UNORM 		16		//2
#define DLIS_ULONG 		17		//4
#define DLIS_UVARI 		18		//1, 2 or 4
#define DLIS_IDENT 		19		//V
#define DLIS_ASCII 		20		//V
#define DLIS_DTIME 		21		//8
#define DLIS_ORIGIN 	22		//V
#define DLIS_OBNAME 	23		//V
#define DLIS_OBJREF 	24		//V
#define DLIS_ATTREF 	25		//V
#define DLIS_STATUS 	26		//1
#define DLIS_UNITS 		27		//V


//representation code length
#define DLIS_FSHORT_LEN 			2
#define DLIS_FSINGL_LEN 			4
#define DLIS_FSING1_LEN 			8
#define DLIS_FSING2_LEN 			12
#define DLIS_ISINGL_LEN 			4
#define DLIS_VSINGL_LEN 			4
#define DLIS_FDOUBL_LEN 			8
#define DLIS_FDOUB1_LEN 			16
#define DLIS_FDOUB2_LEN 			24
#define DLIS_CSINGL_LEN 			8
#define DLIS_CDOUBL_LEN 			16
#define DLIS_SSHORT_LEN 			1
#define DLIS_SNORM_LEN				2
#define DLIS_SLONG_LEN 				4
#define DLIS_USHORT_LEN 			1
#define DLIS_UNORM_LEN 				2
#define DLIS_ULONG_LEN 				4
#define DLIS_UVARI_LEN 				0 	//1, 2 or 4
#define DLIS_IDENT_LEN 				0	//V
#define DLIS_ASCII_LEN 				0	//V
#define DLIS_DTIME_LEN 				8
#define DLIS_ORIGIN_LEN 			0	//V
#define DLIS_OBNAME_LEN 			0	//V
#define DLIS_OBJREF_LEN 			0	//V
#define DLIS_ATTREF_LEN 			0	//V
#define DLIS_STATUS_LEN 			1
#define DLIS_UNITS_LEN 				0	//V


#endif
