#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "circuit.h"

static Element get_element(char string[], int size);

int parse_file(const char* filename, ElementDynArray* dynamic_element_array) {
    // Open file
    FILE* netlist_file = fopen(filename, "r");
    // In case file doesn't exist
    if (netlist_file == NULL) {
        return 1;
    }

    // Buffer
    char line[LINE_SIZE];

    // For each line of input or until we hit the EOF
    while (fgets(line, sizeof(line), netlist_file)) {
        // If line starts with .end the netlist is finished
        if (strncmp(line, ".end", 4) == 0) {
            break;
        }

        // Based on line from file, separate data and create a new element
        Element element = get_element(line, LINE_SIZE);

        // In case of an error element
        if (element.node_pos == -1) {
            continue;
        }
        // Add element to array and close the file is something goes wrong
        if (add_element(dynamic_element_array, &element)) {
            fclose(netlist_file);
            return 3;
        }
    }

    // Close file on success
    fclose(netlist_file);
    return 0;
}

// Convert string in form - (ELEMENT_NAME NODE+ NODE- VALUE) to a circuit element
Element get_element(char string[], int size) {
    // We'll process the string a little before checking for data
    char processed_string[size];
    
    char element_name[ELEMENT_NAME_LENGTH];
    char element_type;

    int node_pos; 
    int node_neg; 
    
    double value;

    // Basically copy the original string and exit for * which represent comments
    for (int i = 0; i < size; i++) {
        // If a line has a comment it doesn't have a valid circuit element
        if (string[i] == '*') {
            return ERROR_ELEMENT;
        }
        // Skip the line break and add NUL terminator instead
        if (string[i] == '\n') {
            processed_string[i] = '\0';
            break;
        }

        processed_string[i] = string[i];
    }
    // Add another NUL terminator at the very end of the buffer just in case the string to be processed is too long
    processed_string[size - 1] = '\0';

    // See how many parts of the data we need could be parsed on the processed line when trying to assign 
    int result = sscanf(processed_string, "%3s %i %i %lf", element_name, &node_pos, &node_neg, &value);

    // Something's wrong with the line format
    if (result != 4) {
        return ERROR_ELEMENT;
    }

    // The type of a circuit element is essentially the first letter of the element name
    element_type = element_name[0];

    // Initialize our output
    Element output_element = {
        .name = "   ",
        .type = element_type,
        .node_pos = node_pos,
        .node_neg = node_neg,
        .value = value
    };

    // Copy name to output element and return the circuit element
    strcpy(output_element.name, element_name);

    return output_element;
}