#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "futhark_parser.h"

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

int main() {
    struct data_format *inputdata = NULL;
    parse("data.in", &inputdata, 1);
}
