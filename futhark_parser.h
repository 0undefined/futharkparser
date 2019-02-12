#ifndef FUTHARK_PARSER
#define FUTHARK_PARSER

// Futhark variable types
typedef enum {
    I8,
    I16,
    I32,
    I64,
    U8,
    U16,
    U32,
    U64,
    F32,
    F64,
    Bool,
} Type;

// Struct to hold information about the variable type
// length = 0 : single value
// length > 0 : array of n variables
typedef struct {
    void *data;
    Type type;
    size_t length;
    size_t *dimensions;
    size_t n_dimensions;
} Format;

typedef struct {
    Format *data;
    size_t length;
} DataObj;

typedef char byte;

void print_topology(const char *filename);
void print_data(DataObj **in);
void parse(const char *filename, DataObj **in);

#endif
