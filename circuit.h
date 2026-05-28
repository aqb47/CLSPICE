// Contains circuit, element definitions and initialization functions

#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_CAPACITY 5
#define ERROR_ELEMENT (Element) {.name=" ", .type=' ', .node_neg=-1, .node_pos=-1, .value=-1}
#define ERROR_DYNARRAY (ElementDynArray){.capacity = 0, .size = 0, .element_array = NULL}
#define ELEMENT_NAME_LENGTH 4

// Elements/ branches that constitute the circuit
typedef struct {
    char name[ELEMENT_NAME_LENGTH]; // "R1" / "V1"
    char type; // 'V' / 'I' / 'R', essentially name[0]
    
    // Nodes are going to be whole numbers where 0 is GND (no node names)
    int node_pos; // Node from
    int node_neg; // Node to
    
    double value;
} Element;

// Dynamic element array
typedef struct {
    int size; // Elements in array currently
    int capacity; // Max possible elements in array

    Element* element_array; // Pointer to array
} ElementDynArray;

// Circuit itself
typedef struct {
    int node_number; // Total nodes including GND
    int voltage_source_number; // Total independent ideal voltage sources
    
    ElementDynArray elements; // Elements array - tell us total elements too
} Circuit;

ElementDynArray elementDynArray_init(int capacity);
void elementDynArray_free(ElementDynArray* dynamic_element_array);
int add_element(ElementDynArray* dynamic_element_array, Element* element);

int get_node_number(ElementDynArray dynamic_element_array);
int get_voltage_source_number(ElementDynArray dynamic_element_array);

// Testing functions
void print_element(Element element);
void print_dynamic_element_array(ElementDynArray dynamic_element_array);
void print_circuit(Circuit circuit);

#endif