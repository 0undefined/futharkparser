#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "futhark_parser.h"

int main() {
    DataObj *inputdata = NULL;
    //print_heuristics("data.in");
    //print_topology("data.in");
    parse("data.in", &inputdata);
    print_data(&inputdata);
}
