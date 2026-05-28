#include <stdlib.h>
#include "circuit.h"

static int elementDynArray_resize(ElementDynArray* dynamic_element_array);

ElementDynArray elementDynArray_init(int capacity) {
    ElementDynArray output_array;

    if (capacity < 0) {
        capacity = DEFAULT_CAPACITY;
    }

    output_array.element_array = malloc(sizeof(Element) * capacity);
    
    if (output_array.element_array == NULL) {
        return ERROR_DYNARRAY;
    }

    output_array.size = 0;
    output_array.capacity = capacity;

    return output_array;
}

void elementDynArray_free(ElementDynArray* dynamic_element_array) {
    free(dynamic_element_array->element_array);

    dynamic_element_array->element_array = NULL;
    dynamic_element_array->size = 0;
    dynamic_element_array->capacity = 0;
}

static int elementDynArray_resize(ElementDynArray* dynamic_element_array) {
    Element* temp = realloc(dynamic_element_array->element_array, 2 * sizeof(Element) * dynamic_element_array->capacity);

    if (temp == NULL) {
        return 1;
    }

    dynamic_element_array->element_array = temp;
    dynamic_element_array->capacity *= 2;

    return 0;
}

int add_element(ElementDynArray* dynamic_element_array, Element* element) {
    if (dynamic_element_array->size == dynamic_element_array->capacity) {
        if (elementDynArray_resize(dynamic_element_array)) {
            return 1;
        }
    }

    dynamic_element_array->element_array[dynamic_element_array->size] = *element;
    dynamic_element_array->size += 1;

    return 0;
}

int get_node_number(ElementDynArray dynamic_element_array) {
    int max = 0;

    for (int i = 0; i < dynamic_element_array.size; i++) {
        int node_pos = dynamic_element_array.element_array[i].node_pos;
        int node_neg = dynamic_element_array.element_array[i].node_neg;

        int current_max = (node_pos > node_neg) ? node_pos : node_neg;

        if (current_max > max) {
            max = current_max;
        }
    }

    return max + 1;
}

int get_voltage_source_number(ElementDynArray dynamic_element_array) {
    int count = 0;

    for (int i = 0; i < dynamic_element_array.size; i++) {
        if (dynamic_element_array.element_array[i].type == 'V') {
            count += 1;
        }
    }

    return count;
}

void print_element(Element element) {
    printf("%s (type %c) %i %i %lf\n", element.name, element.type, element.node_pos, element.node_neg, element.value);
}

void print_dynamic_element_array(ElementDynArray dynamic_element_array) {
    for (int i = 0; i < dynamic_element_array.size; i++) {
        print_element(dynamic_element_array.element_array[i]);
    }
}

void print_circuit(Circuit circuit) {
    print_dynamic_element_array(circuit.elements);

    printf("Nodes: %i | Voltage Sources: %i", circuit.node_number, circuit.voltage_source_number);
}
