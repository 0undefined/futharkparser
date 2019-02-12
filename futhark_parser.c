#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "futhark_parser.h"

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

// Gosh i hate long-ass switch statements
const char* typestring(Type t) {
    // The leading space is because the binary is written like this. blame
    // Troels
    switch(t) {
        case I8:
            return "  i8";
            break;
        case I16:
            return " i16";
            break;
        case I32:
            return " i32";
            break;
        case I64:
            return " i64";
            break;
        case U8:
            return "  u8";
            break;
        case U16:
            return " u16";
            break;
        case U32:
            return " u32";
            break;
        case U64:
            return " u64";
            break;
        case F32:
            return " f32";
            break;
        case F64:
            return " f64";
            break;
        case Bool:
            return "bool";
            break;
        default:
            return "Error";
            break;
    }
    return "Error";
}

// Struct to hold information about the variable type
// length = 0 : single value
// length > 0 : array of n variables
typedef struct {
    void *data;
    Type type;
    size_t length;
} Format;

typedef struct {
    Format *data;
    size_t length;
} DataObj;

void print_data(DataObj **in) {
    for(int i = 0; i < (int)(*in)->length; i++) {
        Type t = (*in)->data[i].type;
        size_t len = (*in)->data[i].length;

        printf("%c: ", 'a' + i);

        if(len > 0) {
            printf("%s[%lu]\n", typestring(t), len);
        } else {
            void *dat = (*in)->data[i].data;
            switch(t) {
                case(I8):
                case(I16):
                case(I32):
                    printf("%d", *(int*)dat);
                    break;
                case(I64):
                    printf("%lld", *(long long*)dat);
                    break;
                case(U8):
                case(U16):
                case(U32):
                    printf("%u", *(unsigned*)dat);
                    break;
                case(U64):
                    printf("%llu", *(long long unsigned*)dat);
                    break;
                case(F32):
                case(F64):
                    printf("%.04f",  *(double*)dat);
                    break;
                case(Bool):
                    printf("%s", (*(unsigned char*)dat != 0)?"True":"False");
                    break;
                default:
                    // Error
                    fprintf(stderr, "\n\rSomething went wrong trying to recognize the "
                            "datatype for the %dth variable.\n", i);
                    break;
            }
            printf("%s\n", typestring(t));
        }
    }
    printf("\n");
}

// Returns 1 if b is whitespace
byte is_whitespace(byte b) {
    if(b == ' ' || b == 0x00 || b == '\t' || b == '\n') return 1;
    return 0;
}

void fstep_back_pos(FILE *f) {
    // This is a weak and lazy way of getting errors, but we want to avoid a
    // long if/then/else chain for a mere 4 statements
    int errs = 0;
    fpos_t p;
    errs |= fgetpos(f, &p);
    p.__pos -= 1;
    errs |= fsetpos(f, &p);
    errs |= fgetpos(f, &p); // and update the old one
    if(errs) {
        fprintf(stderr, "Set/Get-fpos Error! (%d)\n%s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void fstep_skip_bytes(size_t n_bytes, FILE *f) {
    int errs = 0;
    fpos_t p;
    errs |= fgetpos(f, &p);
    p.__pos += n_bytes;
    errs |= fsetpos(f, &p);
    errs |= fgetpos(f, &p); // and update the old one
    if(errs) {
        fprintf(stderr, "Set/Get-fpos Error! (%d)\n%s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void print_topology(const char *filename) {
    // Open file
    FILE *f = fopen(filename, "rb");
    if(errno) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // Read file
    fpos_t r;
    // yeah, only 1 byte buffer idk
    byte buffer = 0;

    // Variable count
    int counter = 0;

    while(buffer != EOF && !feof(f)) {
        // Skip whitespace
        do { buffer = fgetc(f); }
        while(buffer != EOF && is_whitespace(buffer));

        if(buffer == EOF) {
            if(feof(f)) break;
            fgetpos(f, &r);
            fprintf(stderr, "Either EOF or something unexpected happened(0x%lx)\n", r.__pos);
            exit(EXIT_FAILURE);
        }
        if(fgetpos(f, &r)) {
            fprintf(stderr, "Error! (%d)\n%s\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }

        // Read the dataset-type (1b)
        // we only support binary
        if(buffer != 'b'){
            fprintf(stderr, "Unsupported filetype.\n");
            fprintf(stderr, "Got type: 0x%x at pos %lx\nExpected: 0x62\n", buffer, r.__pos);
            exit(EXIT_FAILURE);
        }

        // Read the version (we dont support anything else but 2 but it's ignored (1b)
        // anyways
        buffer = fgetc(f);
        if(buffer != 2) {
            fprintf(stderr, "Unsupported version! (only supported version is 2)\n");
            fprintf(stderr, "Got version: %d at pos %lx\nExpected: 0x62\n", buffer, r.__pos);
            exit(EXIT_FAILURE);
        };

        // Read Dimensions -- currently only supporting scalars (aka. 0) (1b)
        int d;
        buffer = fgetc(f);
        if(buffer == EOF) {
            fprintf(stderr, "Unexpected EOF\n");
            exit(EXIT_FAILURE);
        }
        d = buffer;

        // Read types (4b)
        // Skip WS
        size_t tmp_n;
        char *vartype = (char*)malloc(sizeof(char) * 4);
        tmp_n = fread(vartype, sizeof(char), 4, f);
        if(tmp_n != 4) {
            fprintf(stderr, "Error reading type!\n");
            exit(EXIT_FAILURE);
        }

        if(!strcmp(" i32", vartype) ||
           !strcmp(" f32", vartype)) {
            vartype++; // fuck that leading whitespace

            size_t typesize = sizeof(int);
            size_t total_array_size = 1;

            while(d > 0) {
                // Dimension-size is 64-bit
                size_t d_size = sizeof(long long); // ;)
                size_t tmp_size;

                tmp_n = fread(&tmp_size, d_size, 1, f);
                if(tmp_n != 1) {
                    fprintf(stderr, "Reading integer failed! (wrong size read)\n");
                    exit(EXIT_FAILURE);
                }
                printf("[%lu]", tmp_size);
                total_array_size *= tmp_size;
                // This line is only to save m/N, remove for better
                // combatability
                //if(d == 2) (*in)->m = tmp_size;
                //else (*in)->N = tmp_size;
                d--;
            };
            printf("%s\n", vartype);

            // Skip the values
            fstep_skip_bytes(typesize*total_array_size, f);

        } else {
            fprintf(stderr, "Unsupported var-type!\n");
            exit(EXIT_FAILURE);
        }

        counter++;
    }
    printf("Total of %d variables.\n", counter);
}

////////////////////////////////////////////////////////////////////////////////
//                                   LEGACY                                   //
////////////////////////////////////////////////////////////////////////////////

// TODO: Change this struct to your needs
struct data_format {
    int *k;
    int *n;
    float *freq;
    float *hfrac;
    float *lam;
    float *images; // m*N size
    size_t m;
    size_t N;
};

typedef struct data_format data_format;

void parse(const char *filename, data_format **in, byte verbose) {
    // Allocate variables
    *in = (data_format*)malloc(sizeof(struct data_format));
    // Open file
    FILE *f = fopen(filename, "rb");
    if(errno) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // Read file
    fpos_t r;
    byte buffer = 0;

    int counter = 0;

    while(buffer != EOF && !feof(f)) {
        // Skip whitespace
        do {
            buffer = fgetc(f);
        } while(buffer != EOF && is_whitespace(buffer));
        if(buffer == EOF) {
            if(feof(f)) break;
            fgetpos(f, &r);
            fprintf(stderr, "Either EOF or something unexpected happened(0x%lx)\n", r.__pos);
            exit(EXIT_FAILURE);
        }
        if(fgetpos(f, &r)) {
            fprintf(stderr, "Error! (%d)\n%s\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }

        // Read the dataset-type (1b)
        // we only support binary
        if(buffer != 'b'){
            fprintf(stderr, "Unsupported filetype.\n");
            fprintf(stderr, "Got type: 0x%x at pos %lx\nExpected: 0x62\n", buffer, r.__pos);
            exit(EXIT_FAILURE);
        }
        if(verbose) printf("Binary dataset detected\n");
        // Read the version (we dont support anything else but 2 but it's ignored (1b)
        // anyways
        buffer = fgetc(f);
        if(buffer != 2) {
            fprintf(stderr, "Unsupported version!\n");
            fprintf(stderr, "Got version: %d at pos %lx\nExpected: 0x62\n", buffer, r.__pos);
            exit(EXIT_FAILURE);
        };
        if(verbose) printf("Version 2 detected\n");
        // Read Dimensions -- currently only supporting scalars (aka. 0) (1b)
        int d;
        buffer = fgetc(f);
        if(buffer == EOF) {
            fprintf(stderr, "Unexpected EOF\n");
            exit(EXIT_FAILURE);
        }
        d = buffer;
        if(verbose) printf("Dimensions: %d\n", d);

        // Read types (4b)
        // Skip WS
        size_t tmp_n;
        char *vartype = (char*)malloc(sizeof(char) * 4);
        tmp_n = fread(vartype, sizeof(char), 4, f);
        if(tmp_n != 4) {
            fprintf(stderr, "Error reading type!\n");
            exit(EXIT_FAILURE);
        }

        if(!strcmp(" i32", vartype) ||
           !strcmp(" f32", vartype)) {
            vartype++; // fuck that leading whitespace

            size_t typesize = sizeof(int);
            size_t total_array_size = 1;

            while(d > 0) {
                // Dimension-size is 64-bit
                size_t d_size = sizeof(long long); // ;)
                size_t tmp_size;

                tmp_n = fread(&tmp_size, d_size, 1, f);
                if(tmp_n != 1) {
                    fprintf(stderr, "Reading integer failed! (wrong size read)\n");
                    exit(EXIT_FAILURE);
                }
                if(verbose) printf("[%lu]", tmp_size);
                total_array_size *= tmp_size;
                // This line is only to save m/N, remove for better
                // combatability
                if(d == 2) (*in)->m = tmp_size;
                else (*in)->N = tmp_size;
                d--;
            };
            if(verbose) {
                if(total_array_size > 1)
                    printf("%s (%lu)\n", vartype, total_array_size);
                else printf("%s\n", vartype);
            }

            // Read the values
            // This is where we would have liked a cleaner solution
            // The only changed thing between if-statements are the value stored

            // TODO: change these accordingly
            switch(counter) {
              case 0:
                (*in)->k = (int*)malloc(typesize*total_array_size);
                tmp_n = fread((*in)->k, typesize, total_array_size, f);
                break;
              case 1:
                (*in)->n = (int*)malloc(typesize*total_array_size);
                tmp_n = fread((*in)->n, typesize, total_array_size, f);
                break;
              case 2:
                (*in)->freq = (float*)malloc(typesize*total_array_size);
                tmp_n = fread((*in)->freq, typesize, total_array_size, f);
                break;
              case 3:
                (*in)->hfrac = (float*)malloc(typesize*total_array_size);
                tmp_n = fread((*in)->hfrac, typesize, total_array_size, f);
                break;
              case 4:
                (*in)->lam = (float*)malloc(typesize*total_array_size);
                tmp_n = fread((*in)->lam, typesize, total_array_size, f);
                break;
              case 5:
                (*in)->images = (float*)malloc(typesize*total_array_size);
                tmp_n = fread((*in)->images, typesize, total_array_size, f);
                if(verbose) printf("items read: %lu\n", tmp_n);
                break;
              default:
                fgetpos(f, &r);
                fprintf(stderr, "Something went wrong!\nfile at %lu", r.__pos);
                exit(EXIT_FAILURE);
            }
            if(tmp_n != total_array_size) {
                fprintf(stderr, "Reading %s failed! (wrong size read)\n", vartype);
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "Unsupported var-type!\n");
            exit(EXIT_FAILURE);
        }

        counter++;
    }

    if(verbose) {
        printf("k: %d\n", *(*in)->k);
        printf("n: %d\n", *(*in)->n);
        printf("freq: %.02f\n",  (*in)->freq);
        printf("hfrac: %.02f\n", (*in)->hfrac);
        printf("lam: %.02f\n",   (*in)->lam);
        printf("m: %lu\tN: %lu\n", (*in)->m, (*in)->N);
        printf("first 5 pixels:\n");
        for(int i = 0; i< 5; i++) printf("%.02f\t", (*in)->images[i]);
        printf("\nlast 5 pixels:\n");
        for(int i = sizeof((*in)->images)-5; i < sizeof((*in)->images); i++) printf("%.02f\t", (*in)->images[i]);

        printf("\n");
    }
}
