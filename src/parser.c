#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "parser.h"
#include "circuit.h"

static Element get_element(char string[], int size);
static double parse_string(char string[], int size);

int parse_file(const char* filename, ElementDynArray* dynamic_element_array) {
    // Open file
    FILE* netlist_file = fopen(filename, "r");
    // In case file doesn't exist
    if (netlist_file == NULL) {
        printf("Couldn't open netlist file");

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
        if (element.node_pos == ERROR_ELEMENT.node_pos) {
            continue;
        }

        // Add element to array and close the file is something goes wrong
        if (ElementDynArray_add(dynamic_element_array, &element)) {
            printf("Couldn't add element\n");

            fclose(netlist_file);
            
            return 2;
        }
    }

    // Close file on success
    fclose(netlist_file);
    
    return 0;
}

// Convert string in form - (ELEMENT_NAME NODE+ NODE- VALUE) to a normal circuit element
// Or convert string in form - (ELEMENT_NAME NODE+ NODE- CTRL_NODE+ CTRL_NODE- GAIN) to voltage-controlled element
// Or convert string in form - (ELEMENT_NAME NODE+ NODE- CTRL_ELEMENT_NAME GAIN) to current-controlled element
Element get_element(char string[], int size) {
    // We'll process the string a little before checking for data in this processed string array
    char processed_string[size];
    memset(processed_string, 0, size);
    
    // Element specific values
    char element_name[ELEMENT_NAME_LENGTH];
    char element_type;

    int node_pos; 
    int node_neg; 
    
    int ctrl_node_pos;
    int ctrl_node_neg;
    char ctrl_name[ELEMENT_NAME_LENGTH];

    double value;

    // End pointer for strtol
    char* end_ptr;
    
    // Value as string for parsing last character suffix
    char value_string[VALUE_SIZE];

    // Basically copy the original string and exit for * which represent comments
    for (int i = 0; i < size; i++) {
        // Skip line breaks and comments that start with *
        if (string[i] == '*' || string[i] == '\n' || string[i] == '\0') {
            processed_string[i] = '\0';
            break;
        }

        processed_string[i] = string[i];
    }

    // If processed string is empty no need to analyze the netlist
    if (strlen(processed_string) == 0) {
        return ERROR_ELEMENT;
    }

    // Add another NUL terminator at the very end of the buffer just in case the string to be processed is too long
    processed_string[size - 1] = '\0';

    // Tokenize string
    char* word_ptr = strtok(processed_string, " ");

    // Get element name and element type as firs letter of the element name
    if (strlen(word_ptr) > ELEMENT_NAME_LENGTH - 1) {
        printf("Element name too long to be processed");
        return ERROR_ELEMENT;
    }

    strcpy(element_name, word_ptr);
    element_type = element_name[0];

    // Get positive node
    word_ptr = strtok(NULL, " ");
    if (word_ptr == NULL) {
        printf("Missing positive node\n");
        return ERROR_ELEMENT;
    }

    long temp_node_pos = strtol(word_ptr, &end_ptr, 10);
    if (temp_node_pos > INT_MAX || temp_node_pos < 0) {
        printf("Positive node integer out of range\n");
        return ERROR_ELEMENT;
    }

    node_pos = (int) temp_node_pos;

    // Get negative node
    word_ptr = strtok(NULL, " ");
    if (word_ptr == NULL) {
        printf("Missing negative node\n");
        return ERROR_ELEMENT;
    }

    long temp_node_neg = strtol(word_ptr, &end_ptr, 10);
    if (temp_node_neg > INT_MAX || temp_node_neg < 0) {
        printf("Negative node integer out of range\n");
        return ERROR_ELEMENT;
    }

    node_neg = (int) temp_node_neg;

    // We have a voltage controlled dependent source which should have 6 total inputs 
    if (element_name[0] == 'E' || element_name[0] == 'G') {
        // Get positive control node
        word_ptr = strtok(NULL, " ");
        if (word_ptr == NULL) {
            printf("Missing positive control node\n");
            return ERROR_ELEMENT;
        }

        long temp_ctrl_node_pos = strtol(word_ptr, &end_ptr, 10);
        if (temp_ctrl_node_pos > INT_MAX || temp_ctrl_node_pos < 0) {
            printf("Positive control node integer out of range\n");
            return ERROR_ELEMENT;
        }

        ctrl_node_pos = (int) temp_ctrl_node_pos;

        // Get negative control node
        word_ptr = strtok(NULL, " ");
        if (word_ptr == NULL) {
            printf("Missing negative control node\n");
            return ERROR_ELEMENT;
        }

        long temp_ctrl_node_neg = strtol(word_ptr, &end_ptr, 10);

        if (temp_ctrl_node_neg > INT_MAX || temp_ctrl_node_neg < 0) {
            printf("Negative control node integer out of range\n");
            return ERROR_ELEMENT;
        }

        ctrl_node_neg = (int) temp_ctrl_node_neg; 
        
        // Set control name
        strcpy(ctrl_name, NON_CC_ELEMENT_NAME);
    }

    // We have a current controlled dependent source which should have 5 total inputs
    else if (element_name[0] == 'F' || element_name[0] == 'H') {
        // Set positive and negative control nodes
        ctrl_node_pos = NON_VC_ELEMENT_NODE;
        ctrl_node_neg = NON_VC_ELEMENT_NODE;

        // Get control name
        word_ptr = strtok(NULL, " ");
        if (word_ptr == NULL) {
            printf("Missing control name\n");
            return ERROR_ELEMENT;
        }

        if (strlen(word_ptr) > ELEMENT_NAME_LENGTH - 1) {
            printf("Control element name is too long\n");
            return ERROR_ELEMENT;
        }

        strcpy(ctrl_name, word_ptr);
    }

    // Normal element
    else {
        // Set ctrl nodes and name
        ctrl_node_pos = NON_VC_ELEMENT_NODE;
        ctrl_node_neg = NON_VC_ELEMENT_NODE;
        strcpy(ctrl_name, NON_CC_ELEMENT_NAME);
    }

    // Get element value
    word_ptr = strtok(NULL, " ");
    if (word_ptr == NULL) {
        printf("Missing element value\n");
        return ERROR_ELEMENT;
    }

    if (strlen(word_ptr) > VALUE_SIZE - 1) {
        printf("Value of element has too many digits\n");
        return ERROR_ELEMENT;
    }

    strcpy(value_string, word_ptr);

    // Same thing we did for processed string
    value_string[VALUE_SIZE - 1] = '\0';

    // Parse value string. +1 for NUL terminator
    value = parse_string(value_string, strlen(value_string) + 1);
    if (isnan(value)) {
        return ERROR_ELEMENT;
    }

    // Initialize our output
    Element output_element = {
        .name = "   ",
        .type = element_type,

        .node_pos = node_pos,
        .node_neg = node_neg,

        .ctrl_node_pos = ctrl_node_pos,
        .ctrl_node_neg = ctrl_node_neg,

        .value = value
    };

    // Copy name, control name to output element and return the circuit element
    strcpy(output_element.name, element_name);
    strcpy(output_element.ctrl_name, ctrl_name);

    return output_element;
}

// Convert a string array with an SI unit suffix to a double. The size should include the NUL terminator
double parse_string(char string[], int size) {
    // The copy of the string we'll work with so original string isn't changed
    char string_copy[size];
    strcpy(string_copy, string);

    // If there is a suffix it'll be at the last character of the string ahead of NUL terminator
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
        // Nano
        if (tolower(suffix) == 'n') {
            value *= 1e-9;
        }
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