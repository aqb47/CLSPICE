#include "circuit.h"
#include "mmna.h"

static int stamp_resistor(Element element, SparseMatrix_COO* input_sparse_matrix);
static int stamp_independent_current_source(Element element, DenseVector* output_dense_vector);
static int stamp_independent_voltage_source(Element element, SparseMatrix_COO* input_sparse_matrix, DenseVector* output_dense_vector, int* ivs_counter, int total_nodes);

int build_input_output_sparsematrix(SparseMatrix_COO* input, DenseVector* output, Circuit circuit) {
    ElementDynArray dynamic_element_array = circuit.elements;
    int node_number = circuit.node_number;
    int voltage_source_number = circuit.voltage_source_number;

    int variable_count = node_number + voltage_source_number - 1;

    // Check for valid usage
    if (dynamic_element_array.element_array == NULL) {
        printf("No elements to build array!\n");
        return 1;
    }
    else if (node_number == -1) {
        printf("Invalid node number!\n");
        return 2;
    }
    else if (voltage_source_number == -1) {
        printf("Invalid voltage source number!\n");
        return 3;
    }
    else if (variable_count != input->rows || variable_count != input->cols) {
        printf("Invalid dimensions for input matrix!\n");
        return 4;
    }
    else if (variable_count != output->size) {
        printf("Invalid dimensions for output vector!\n");

        return 5;
    }

    int voltage_source_counter = 0;

    // Go through every element of dynamic element array and stamp that element
    for (int i = 0; i < dynamic_element_array.size; i++) {
        Element current_element = dynamic_element_array.element_array[i];

        int return_code;

        // Resistor
        if (current_element.type == 'R') {
            return_code = stamp_resistor(current_element, input);
        }
        // Independent current source
        else if (current_element.type == 'I') {
            return_code = stamp_independent_current_source(current_element, output);
        }
        // Independent voltage source
        else if (current_element.type == 'V') {
            return_code = stamp_independent_voltage_source(current_element, input, output, &voltage_source_counter, node_number);
        }
        // Unknown component
        else {
            return 6;
        }

        // If something went wrong
        if (return_code) {
            return 7;
        }
    }

    return 0;
}

// Stamp a DC fixed resistance resistor
int stamp_resistor(Element element, SparseMatrix_COO* input_sparse_matrix) {
    double resistance = element.value;

    // In case of a short circuit
    if (fabs(resistance) < 1e-14) {
        return 1;
    }

    int node_pos = element.node_pos;
    int node_neg = element.node_neg;

    int return_code = 0;

    // Add conductance at diagonal entries of conductance matrix 
    if (node_pos != 0) {
        return_code += SparseMatrix_COO_add_entry(input_sparse_matrix, node_pos - 1, node_pos - 1, +1 / resistance);
    }
    if (node_neg != 0) {
        return_code += SparseMatrix_COO_add_entry(input_sparse_matrix, node_neg - 1, node_neg - 1, +1 / resistance);
    }

    // Add negative of conductance at non-diagonal entries of conductance matrix
    if (node_pos != 0 && node_neg != 0) {
        return_code += SparseMatrix_COO_add_entry(input_sparse_matrix, node_pos - 1, node_neg - 1, -1 / resistance);
        return_code += SparseMatrix_COO_add_entry(input_sparse_matrix, node_neg - 1, node_pos - 1, -1 / resistance);
    }

    if (return_code) {
        return 2;
    }

    return 0;
}

// Stamp a DC independent ideal current source
int stamp_independent_current_source(Element element, DenseVector* output_dense_vector) {
    double current = element.value;
    int node_pos = element.node_pos;
    int node_neg = element.node_neg;

    int return_code = 0;

    // Decrement current for non-zero +node for entries of currrent source current vector I
    if (node_pos != 0) {
        return_code += DenseVector_add_entry(output_dense_vector, node_pos - 1, -current);
    }

    // Increment current for non-zero -node for entries of current source current vector I
    if (node_neg != 0) {
        return_code += DenseVector_add_entry(output_dense_vector, node_neg - 1, +current);
    }

    if (return_code) {
        return 1;
    }

    return 0;
}

// Stamp a DC independent ideal voltage source
int stamp_independent_voltage_source(Element element, SparseMatrix_COO* input_sparse_matrix, DenseVector* output_dense_vector, int* ivs_counter, int total_nodes) {
    double voltage = element.value;
    int node_pos = element.node_pos;
    int node_neg = element.node_neg;

    int return_code = 0;

    // +1 for non-zero +node to incidence matrix B and it's transpose C
    if (node_pos != 0) {
        // B
        return_code += SparseMatrix_COO_add_entry(input_sparse_matrix, node_pos - 1,  total_nodes - 1 + *ivs_counter, +1);
        // C
        return_code += SparseMatrix_COO_add_entry(input_sparse_matrix, total_nodes - 1 + *ivs_counter, node_pos - 1, +1);
    }

    // -1 for non-zero -node to incidence matrix B and it's transpose C
    if (node_neg != 0) {
        // B
        return_code += SparseMatrix_COO_add_entry(input_sparse_matrix, node_neg - 1,  total_nodes - 1 + *ivs_counter, -1);
        // C
        return_code += SparseMatrix_COO_add_entry(input_sparse_matrix, total_nodes - 1 + *ivs_counter,  node_neg - 1, -1);
    }

    // Add voltage to voltage source voltage vector
    return_code += DenseVector_add_entry(output_dense_vector, total_nodes - 1 + *ivs_counter, +voltage);

    // Increment independent voltage source counter
    *ivs_counter += 1;

    if (return_code) {
        return 1;
    }

    return 0;
}