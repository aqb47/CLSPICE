#include "circuit.h"
#include "parser.h"
#include "mna.h"
#include "matrix.h"
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
    Matrix input_matrix;
    Matrix output_matrix;

    // Build matrices
    if (build_input_output_matrix(&input_matrix, &output_matrix, my_circuit)) {
        printf("Matrix Building ERROR\n");
        return 3;
    }

    // Get result 
    Matrix result = gaussian_elimination(input_matrix, output_matrix);
    
    if (result.rows == 0 || result.cols == 0) {
        printf("Solving ERROR\n");
        return 4;
    }

    // Show result
    format_result(result, my_circuit);

    // Free dynamic array
    elementDynArray_free(&my_elements);
    return 0;
}