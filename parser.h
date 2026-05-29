// Functions for parsing netlist into circuit elements

#ifndef PARSER_H
#define PARSER_H

// Size of line buffer during parsing
#define LINE_SIZE 255

#include <stdio.h>
#include <string.h>
#include "circuit.h"

// Parse a netlist input and save each element to dynamic array
int parse_file(const char* filename, ElementDynArray* dynamic_element_array);

#endif