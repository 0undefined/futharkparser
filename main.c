#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

typedef unsigned char byte;

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

struct InPutFormat {
    int *k;
    int *n;
    float *freq;
    float *hfrac;
    float *lam;
    float *images; // m*N size
    size_t m;
    size_t N;
};

void parse(const char *filename, byte verbose) {
    // Size variables
    int m, N;
    // Allocate variables
    struct InPutFormat *in = malloc(sizeof(struct InPutFormat));
    // Open file
    FILE *f = fopen("data.in", "rb");
    if(errno) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // Read file
    fpos_t r;
    byte buffer = 0;//(byte*)malloc(sizeof(byte) * 1024);
    byte format;
    // buffer[0] = 0x00;

    int counter = 0;

    while(buffer != EOF) {
        // Skip whitespace
        do {
            buffer = fgetc(f);
        } while(buffer != EOF && is_whitespace(buffer));
        if(buffer == EOF) {
            fprintf(stderr, "Either EOF or something unexpected happened\n");
            exit(EXIT_FAILURE);
        }
        if(fgetpos(f, &r)) {
            fprintf(stderr, "Error! (%d)\n%s\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }

        // Read the dataset-type (1b)
        // we only support binary
        if(buffer != 'b'){
            if(feof(f)) break;
            fprintf(stderr, "Unsupported filetype.\n");
            fprintf(stderr, "Got type: 0x%x at pos %x\nExpected: 0x62\n", buffer, r);
            exit(EXIT_FAILURE);
        }
        if(verbose) printf("Binary dataset detected\n");
        // Read the version (we dont support anything else but 2 but it's ignored (1b)
        // anyways
        buffer = fgetc(f);
        if(buffer != 2) {
            fprintf(stderr, "Unsupported version!\n");
            fprintf(stderr, "Got version: %d at pos %x\nExpected: 0x62\n", buffer, r);
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
        char *vartype = malloc(sizeof(char) * 4);
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
                if(verbose) printf("[%llu]", tmp_size);
                total_array_size *= tmp_size;
                // This line is only to save m/N, remove for better
                // combatability
                if(d == 2) in->m = tmp_size;
                else in->N = tmp_size;
                d--;
            };
            if(verbose)
                if(total_array_size > 1)
                    printf("%s (%llu)\n", vartype, total_array_size);
                else printf("%s\n", vartype);

            // Read the values
            // This is where we would have liked a cleaner solution
            // The only changed thing between if-statements are the value stored

            switch(counter) {
              case 0:
                in->k = malloc(typesize*total_array_size);
                tmp_n = fread(in->k, typesize, total_array_size, f);
                break;
              case 1:
                in->n = malloc(typesize*total_array_size);
                tmp_n = fread(in->n, typesize, total_array_size, f);
                break;
              case 2:
                in->freq = malloc(typesize*total_array_size);
                tmp_n = fread(in->freq, typesize, total_array_size, f);
                break;
              case 3:
                in->hfrac = malloc(typesize*total_array_size);
                tmp_n = fread(in->hfrac, typesize, total_array_size, f);
                break;
              case 4:
                in->lam = malloc(typesize*total_array_size);
                tmp_n = fread(in->lam, typesize, total_array_size, f);
                break;
              case 5:
                in->images = malloc(typesize*total_array_size);
                tmp_n = fread(in->images, typesize, total_array_size, f);
                if(verbose) printf("items read: %llu\n", tmp_n);
                break;
              default:
                fgetpos(f, &r);
                fprintf(stderr, "Something went wrong!\nfile at %d", r);
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
        printf("k: %d\n", *in->k);
        printf("n: %d\n", *in->n);
        printf("freq: %.02f\n", *in->freq);
        printf("hfrac: %.02f\n", *in->hfrac);
        printf("lam: %.02f\n", *in->lam);
        printf("m: %llu\tN: %llu\n", in->m, in->N);
        printf("first 5 pixels:\n");
        for(int i = 0; i< 5; i++) printf("%.02f\t", in->images[i]);
        printf("\nlast 5 pixels:\n");
        for(int i = sizeof(in->images)-6; i < sizeof(in->images); i++) printf("%.02f\t", in->images[i]);

        printf("\n");
    }
}
