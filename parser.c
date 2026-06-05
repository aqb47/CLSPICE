#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"
#include "circuit.h"

static Element get_element(char string[], int size);
static double parse_string(char string[], int size);

int parse_file(const char* filename, ElementDynArray* dynamic_element_array) {
    // Open file
    FILE* netlist_file = fopen(filename, "r");
    // In case file doesn't exist
    if (netlist_file == NULL) {
        printf("Couldn't open file!");
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
            printf("Couldn't add element\n");
            fclose(netlist_file);
            return 2;
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
        // Skip line breaks and comments that start with *
        if (string[i] == '*' || string[i] == '\n' || string[i] == '\0') {
            processed_string[i] = '\0';
            break;
        }

        processed_string[i] = string[i];
    }
    // Add another NUL terminator at the very end of the buffer just in case the string to be processed is too long
    processed_string[size - 1] = '\0';

    char value_string[VALUE_SIZE];

    // See how many parts of the data we need could be parsed on the processed line when trying to assign 
    int result = sscanf(processed_string, "%3s %i %i %49s", element_name, &node_pos, &node_neg, value_string);

    // Same thing we did for processed string
    value_string[VALUE_SIZE - 1] = '\0';

    // Something's wrong with the line format
    if (result != 4) {
        printf("Could not read 4 data entries in netlist\n");

        return ERROR_ELEMENT;
    }

    // Parse value string. +1 for NUL terminator
    value = parse_string(value_string, strlen(value_string) + 1);
    if (isnan(value)) {
        printf("Could not parse line in netlist\n");

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

// Convert a string array with an SI unit suffix to a double. The size should include the NUL terminator
double parse_string(char string[], int size) {
    // The copy of the string we'll work with so original string isn't changed
    char string_copy[size];
    strcpy(string_copy, string);

    // If there is a suffix it'll be at the last character of the string
    char suffix = string_copy[size - 2];
    int is_suffix_used = 0;
    char* end;

    // Check if it's actually the suffix and isolate number part of string if so
    if (isalpha(suffix)) {
        string_copy[size - 2] = '\0';
        is_suffix_used = 1;
    }

    double value = strtod(string_copy, &end);
    
    // In case something goes wrong
    if (end == string_copy) {
        return NAN;
    }
    
    if (is_suffix_used) {
        // Micro
        if (tolower(suffix) == 'u') {
            value *= 1e-6;
        }
        // Milli
        else if (suffix == 'm') {
            value *= 1e-3;
        }
        // Kilo
        else if (tolower(suffix) == 'k') {
            value *= 1e3;
        }
        // Mega
        else if (suffix == 'M') {
            value *= 1e6;
        }
        // Giga
        else if (tolower(suffix) == 'g') {
            value *= 1e9;
        }
        // Tera
        else if (tolower(suffix) == 't') {
            value *= 1e12;
        }
        // Unknown suffix
        else {
            printf("Unknown suffix used in value for element\n");

            return NAN;
        }
    }

    return value;
}