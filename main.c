#include "circuit.h"
#include "parser.h"
#include "mmna.h"
#include "sparsematrix.h"
#include "output.h"

int main(void) {
    // Initialize netlist file 
    char* netlist_filename = "netlist.sp";

    // Initialize circuit element array
    ElementDynArray my_elements = elementDynArray_init(DEFAULT_CAPACITY);
    if (my_elements.element_array == NULL) {
        printf("Memory allocation for element ERROR");
        return 1;
    }

    // Parse the netlist
    if (parse_file(netlist_filename, &my_elements)) {
        printf("Parsing ERROR\n");
        return 2;
    }

    // Initialize the circuit with the element array
    int node_number = get_node_number(my_elements);
    int voltage_source_number = get_voltage_source_number(my_elements);

    Circuit my_circuit = {
        .elements = my_elements,
        .node_number = node_number,
        .voltage_source_number = voltage_source_number
    };

    // Initialize matrices
    int variable_count = node_number + voltage_source_number - 1;

    SparseMatrix_COO input_sparse_matrix = SparseMatrix_COO_init(variable_count, variable_count);
    if (input_sparse_matrix.rows == ERROR_SPARSE_MATRIX_COO.rows) {
        printf("Input matrix allocation ERROR\n");
        return 3;
    }

    DenseVector output_dense_vector = DenseVector_init(variable_count);
    if (output_dense_vector.size == ERROR_VECTOR.size) {
        printf("Output vector allocation ERROR\n");

        SparseMatrix_COO_free(&input_sparse_matrix);
        return 4;
    }

    // Build matrices
    if (build_input_output_sparsematrix(&input_sparse_matrix, &output_dense_vector, my_circuit)) {
        printf("Matrix building ERROR\n");

        DenseVector_free(&output_dense_vector);
        SparseMatrix_COO_free(&input_sparse_matrix);
        return 5;
    }

    // Solve the equations
    DenseVector result = SparseMatrix_LU_decomposition(&input_sparse_matrix, &output_dense_vector);

    // Free dynamic array and allocated stuff
    elementDynArray_free(&my_elements);
    DenseVector_free(&output_dense_vector);
    SparseMatrix_COO_free(&input_sparse_matrix);

    return 0;
}