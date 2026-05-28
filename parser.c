#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "circuit.h"

static Element get_element(char string[], int size);

int parse_file(const char* filename, ElementDynArray* dynamic_element_array) {
    FILE* netlist_file = fopen(filename, "r");
    if (netlist_file == NULL) {
        return 1;
    }

    char line[LINE_SIZE];

    while (fgets(line, sizeof(line), netlist_file)) {
        if (strncmp(line, ".end", 4) == 0) {
            break;
        }

        Element element = get_element(line, LINE_SIZE);

        if (element.node_pos == -1) {
            continue;
        }
        if (add_element(dynamic_element_array, &element)) {
            fclose(netlist_file);
            return 3;
        }
    }

    fclose(netlist_file);
    return 0;
}

Element get_element(char string[], int size) {
    char processed_string[size];
    
    char element_name[ELEMENT_NAME_LENGTH];
    char element_type;

    int node_pos; 
    int node_neg; 
    
    double value;

    for (int i = 0; i < size; i++) {
        if (string[i] == '*') {
            return ERROR_ELEMENT;
        }
        if (string[i] == '\n') {
            processed_string[i] = '\0';
            break;
        }

        processed_string[i] = string[i];
    }
    processed_string[size - 1] = '\0';

    int result = sscanf(processed_string, "%3s %i %i %lf", element_name, &node_pos, &node_neg, &value);

    if (result != 4) {
        return ERROR_ELEMENT;
    }

    element_type = element_name[0];

    Element output_element = {
        .name = "   ",
        .type = element_type,
        .node_pos = node_pos,
        .node_neg = node_neg,
        .value = value
    };

    strcpy(output_element.name, element_name);

    return output_element;
}