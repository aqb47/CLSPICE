#include "circuit.h"
#include "parser.h"
#include "mna.h"
#include "matrix.h"
#include "output.h"

int main(void) {
    // Initialize netlist file 
    char* netlist_filename = "netlist.sp";

    // Initialize circuit element array and check for errors
    ElementDynArray my_elements = elementDynArray_init(DEFAULT_CAPACITY);
    if (my_elements.element_array == NULL) {
        printf("Memory allocation for element ERROR");
        return 1;
    }

    // Parse the netlist
    if (parse_file(netlist_filename, &my_elements)) {
        printf("Parsing ERROR\n");

        elementDynArray_free(&my_elements);  
        
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
    Matrix input_matrix = Matrix_init(node_number + voltage_source_number - 1, node_number + voltage_source_number - 1); // Coefficient matrix
    Matrix output_matrix = Matrix_init(node_number + voltage_source_number - 1, 1); // Output vector
    if (input_matrix.data == NULL || output_matrix.data == NULL) {
        printf("Memory allocation for input, output matrices ERROR\n");

        elementDynArray_free(&my_elements);

        return 3;
    }

    // Build matrices and check for errors
    if (build_input_output_matrix(&input_matrix, &output_matrix, my_circuit)) {
        printf("Matrix Building ERROR\n");
            
        elementDynArray_free(&my_elements);
        Matrix_free(&input_matrix);
        Matrix_free(&output_matrix);

        return 4;
    }

    // Get result and check for errors
    Matrix result = gaussian_elimination(input_matrix, output_matrix);
    if (result.data == NULL) {
        printf("Solving ERROR\n");
            
        elementDynArray_free(&my_elements);
        Matrix_free(&input_matrix);
        Matrix_free(&output_matrix);

        return 5;
    }

    // Show result
    format_result(result, my_circuit);

    // Free dynamic stuff
    elementDynArray_free(&my_elements);
    Matrix_free(&input_matrix);
    Matrix_free(&output_matrix);
    Matrix_free(&result);

    return 0;
}