#ifndef FUTHARK_PARSER
#define FUTHARK_PARSER

struct data_format;
typedef char byte;
void print_topology(const char *filename);
void parse(const char *filename, struct data_format **in, byte verbose);

#endif
