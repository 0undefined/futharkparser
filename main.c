#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "futhark_parser.h"

int main() {
    struct data_format *inputdata = NULL;
    //print_heuristics("data.in");
    print_topology("data.in");
    parse("data.in", &inputdata, 0);
}
