#include "circuit.h"
#include "parser.h"
#include "mna.h"
#include "matrix.h"
#include "output.h"

int main(void) {
    // File to build circuit from
    char* netlist_filename = "netlist.sp";

    // Dynamic element array containing elements of the circuit
    ElementDynArray my_elements = ElementDynArray_init(DEFAULT_CAPACITY);
    if (my_elements.element_array == NULL) {
        printf("Memory allocation for element ERROR");
        return 1;
    }

    // Parse the netlist and get elements to build element array
    if (parse_file(netlist_filename, &my_elements)) {
        printf("Parsing ERROR\n");

        ElementDynArray_free(&my_elements);  
        
        return 2;
    }

    // Initialize the circuit and get necessary information from it
    Circuit my_circuit = Circuit_init(my_elements);
    
    int node_number = my_circuit.node_number;
    int voltage_source_number = my_circuit.voltage_source_number;

    // Initialize matrices we'll solve
    Matrix input_matrix = Matrix_init(node_number + voltage_source_number - 1, node_number + voltage_source_number - 1); // Coefficient matrix
    Matrix output_matrix = Matrix_init(node_number + voltage_source_number - 1, 1); // Output vector
    if (input_matrix.data == NULL || output_matrix.data == NULL) {
        printf("Memory allocation for input, output matrices ERROR\n");

        ElementDynArray_free(&my_elements);

        return 3;
    }

    // Build matrices by stamping elements and check for errors
    if (build_input_output_matrix(&input_matrix, &output_matrix, my_circuit)) {
        printf("Matrix Building ERROR\n");
            
        ElementDynArray_free(&my_elements);
        Matrix_free(&input_matrix);
        Matrix_free(&output_matrix);

        return 4;
    }

    // Get result by solving the built matrices and check for errors
    Matrix result = gaussian_elimination(input_matrix, output_matrix);
    if (result.data == NULL) {
        printf("Solving ERROR\n");
            
        ElementDynArray_free(&my_elements);
        Matrix_free(&input_matrix);
        Matrix_free(&output_matrix);

        return 5;
    }

    // Show result containing node voltages, branch currents and voltage source currents if any
    format_result(result, my_circuit);

    // Free dynamic stuff
    ElementDynArray_free(&my_elements);
    Matrix_free(&input_matrix);
    Matrix_free(&output_matrix);
    Matrix_free(&result);

    return 0;
}