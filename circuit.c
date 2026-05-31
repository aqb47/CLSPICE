#include <stdlib.h>
#include "circuit.h"

static int elementDynArray_resize(ElementDynArray* dynamic_element_array);

ElementDynArray elementDynArray_init(int capacity) {
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

void elementDynArray_free(ElementDynArray* dynamic_element_array) {
    // The element array is dynamically allocated memory and has to be freed
    free(dynamic_element_array->element_array);

    // We set everything else to default values
    dynamic_element_array->element_array = NULL;
    dynamic_element_array->size = 0;
    dynamic_element_array->capacity = 0;
}

// Resize dynamic array to twice its initial capacity
static int elementDynArray_resize(ElementDynArray* dynamic_element_array) {
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

int add_element(ElementDynArray* dynamic_element_array, Element* element) {
    // If size and capacity are equal a resize is required
    if (dynamic_element_array->size == dynamic_element_array->capacity) {
        // In case resize fails exit early
        if (elementDynArray_resize(dynamic_element_array)) {
            return 1;
        }
    }

    // Add element to element array and increment size
    dynamic_element_array->element_array[dynamic_element_array->size] = *element;
    dynamic_element_array->size += 1;

    return 0;
}

// Nodes in our SPICE program are numerical only
// Furthermore they should be sequential, one after another. As 0 = GND, this means for n as the highest sequential node number, it also represents number of non-reference nodes
// That is, if we can get the highest (max) node number and add 1 to it, we'll get total reference and non-reference nodes
int get_node_number(ElementDynArray dynamic_element_array) {
    int max = 0;

    // Loop through dynamic array
    for (int i = 0; i < dynamic_element_array.size; i++) {
        // Get both positive and negative nodes
        int node_pos = dynamic_element_array.element_array[i].node_pos;
        int node_neg = dynamic_element_array.element_array[i].node_neg;

        // Identify which one is greater to use for comparison
        int current_max = (node_pos > node_neg) ? node_pos : node_neg;

        // Compare to previous maximum
        if (current_max > max) {
            max = current_max;
        }
    }

    // Add one to max node to get total number of nodes
    return max + 1;
}

// An ideal independent DC voltage source starts with 'V' - so this function looks through the type of every circuit element and matches it with 'V', keeping count of matches
int get_voltage_source_number(ElementDynArray dynamic_element_array) {
    int count = 0;

    // Loop through dynamic element array
    for (int i = 0; i < dynamic_element_array.size; i++) {
        // Check for match
        if (dynamic_element_array.element_array[i].type == 'V') {
            count += 1;
        }
    }

    return count;
}

// Print single element information [format: NAME (type TYPE) NODE+ NODE- VALUE]
void print_element(Element element) {
    printf("%s (type %c) %i %i %lf\n", element.name, element.type, element.node_pos, element.node_neg, element.value);
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
