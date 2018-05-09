#include <stdio.h>
#include "utils.h"
#include "parser.h"
#include <string.h>
#include "constants.h"

#define EXIT_IF_TRUE(x) if ((x)) \\
{ \\
	fprintf(stderr, "ERROR \n"); \\
	exit(-1); \\
}

int main() {
	print_log("hi");
	parser_t parser;
	init_parser(&parser);
	char* file_path = "data/AP_1225in_MREX_E_MAIN.dlis";
    
    parser_parses( &parser, file_path);
    
	return 0;
}
