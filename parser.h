// Functions for parsing netlist 

#ifndef PARSER_H
#define PARSER_H

#define LINE_SIZE 255

#include <stdio.h>
#include <string.h>
#include "circuit.h"

int parse_file(const char* filename, ElementDynArray* dynamic_element_array);

#endif