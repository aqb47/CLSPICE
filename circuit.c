#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "circuit.h"

static int ElementDynArray_resize(ElementDynArray* dynamic_element_array);

ElementDynArray ElementDynArray_init(int capacity) {
    ElementDynArray output_array;

    // Check for correct usage of capacity
    if (capacity < 0) {
        capacity = DEFAULT_CAPACITY;
    }

    // Allocate memory
    output_array.element_array = malloc(sizeof(Element) * capacity);
    
    // In case something goes wrong
    if (output_array.element_array == NULL) {
        return ERROR_DYNARRAY;
    }

    // Initialize metadata
    output_array.size = 0;
    output_array.capacity = capacity;

    return output_array;
}

void ElementDynArray_free(ElementDynArray* dynamic_element_array) {
    // The element array is dynamically allocated memory and has to be freed
    free(dynamic_element_array->element_array);

    // We set everything else to default values
    dynamic_element_array->element_array = NULL;
    dynamic_element_array->size = 0;
    dynamic_element_array->capacity = 0;
}

// Resize dynamic array to twice its initial capacity
static int ElementDynArray_resize(ElementDynArray* dynamic_element_array) {
    // Create temporary element pointer and reallocate memory
    Element* temp = realloc(dynamic_element_array->element_array, 2 * sizeof(Element) * dynamic_element_array->capacity);

    // In case something goes wrong
    if (temp == NULL) {
        return 1;
    }

    // If reallocation is successful use temporary element pointer and double capacity
    dynamic_element_array->element_array = temp;
    dynamic_element_array->capacity *= 2;

    return 0;
}

int ElementDynArray_add(ElementDynArray* dynamic_element_array, Element* element) {
    // If size and capacity are equal a resize is required
    if (dynamic_element_array->size == dynamic_element_array->capacity) {
        // In case resize fails exit early
        if (ElementDynArray_resize(dynamic_element_array)) {
            return 1;
        }
    }

    // Add element to element array and increment size
    dynamic_element_array->element_array[dynamic_element_array->size] = *element;
    dynamic_element_array->size += 1;

    return 0;
}

Circuit Circuit_init(ElementDynArray elements) {
    // Maximum node number found after passing through elements
    int max_node = 0;
    // Total voltage source count found after passing through elements
    int voltage_source_count = 0;

    for (int i = 0; i < elements.size; i++) {
        Element current_element = elements.element_array[i];
        // See current max node
        int current_max_node = current_element.node_pos > current_element.node_neg? current_element.node_pos: current_element.node_neg;

        // Check if it's the max
        if (current_max_node > max_node) {
            max_node = current_max_node;
        }

        // Check if element is a voltage source
        if (current_element.type == 'V' || current_element.type == 'E' || current_element.type == 'H') {
            voltage_source_count += 1;
        }
    }

    Circuit output_circuit = {
        .elements = elements,
        .node_number = max_node + 1,
        .voltage_source_number = voltage_source_count
    };

    return output_circuit;
}

int get_voltage_source_index(Circuit circuit, char* voltage_source_name) {
    int voltage_source_count = 0;

    for (int i = 0; i < circuit.elements.size; i++) {
        Element current_element = circuit.elements.element_array[i];

        if (strcmp(voltage_source_name, current_element.name) == 0) {
            return voltage_source_count;
        }

        if (current_element.type == 'V' || current_element.type == 'E' || current_element.type == 'H') {
            voltage_source_count += 1;
        }
    }

    return -1;
}

// Print single element information [format: NAME (type TYPE) NODE+ NODE- VALUE]
void print_element(Element element) {
    printf("%s (type %c) %i %i %i %i %s %lf\n", element.name, element.type, element.node_pos, element.node_neg, element.ctrl_node_pos, element.ctrl_node_neg, element.ctrl_name, element.value);
}

// Print every circuit element of a dynamic element array based on print_element()
void print_dynamic_element_array(ElementDynArray dynamic_element_array) {
    for (int i = 0; i < dynamic_element_array.size; i++) {
        print_element(dynamic_element_array.element_array[i]);
    }
}

// Print all elements of dynamic element array with print_dynamic_element_array() and circuit metadata
void print_circuit(Circuit circuit) {
    print_dynamic_element_array(circuit.elements);

    printf("Nodes: %i | Voltage Sources: %i", circuit.node_number, circuit.voltage_source_number);
}
