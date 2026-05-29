// Contains circuit, element definitions and initialization functions

#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <stdlib.h>
#include <stdio.h>

// Default capacity for dynamic array of elements
#define DEFAULT_CAPACITY 5

// Element that represents something went wrong during an element function
#define ERROR_ELEMENT (Element) {.name=" ", .type=' ', .node_neg=-1, .node_pos=-1, .value=-1}

// Dynamic array that represents something went wrong during a dynamic array function
#define ERROR_DYNARRAY (ElementDynArray){.capacity = 0, .size = 0, .element_array = NULL}

// Default length for name of every circuit element
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

// Initialize dynamic array for circuit element (uses malloc)
ElementDynArray elementDynArray_init(int capacity);
// Free allocated memory for dynamic array from elementDynArray_init()
void elementDynArray_free(ElementDynArray* dynamic_element_array);
// Add element to dynamic array
int add_element(ElementDynArray* dynamic_element_array, Element* element);

// Get total nodes from circuit element array
int get_node_number(ElementDynArray dynamic_element_array);
// Get total voltage sources from circuit element array
int get_voltage_source_number(ElementDynArray dynamic_element_array);

// Testing functions
void print_element(Element element);
void print_dynamic_element_array(ElementDynArray dynamic_element_array);
void print_circuit(Circuit circuit);

#endif