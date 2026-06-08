// Functions for parsing netlist into circuit elements

#ifndef PARSER_H
#define PARSER_H

// Size of line buffer during parsing
#define LINE_SIZE 255
// Max size of buffer for reading element value
#define VALUE_SIZE 50

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include "circuit.h"

// Parse a netlist input and save each element to dynamic array
int parse_file(const char* filename, ElementDynArray* dynamic_element_array);

#endif