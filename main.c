#include "circuit.h"
#include "parser.h"
#include "mna.h"

int main(void) {
    // Initialize netlist file and circuit element array
    char* netlist_filename = "netlist.sp";
    ElementDynArray my_elements = elementDynArray_init(DEFAULT_CAPACITY);

    // Parse the netlist
    if (parse_file(netlist_filename, &my_elements)) {
        return 1;
    }

    // Initialize the circuit
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
        return 2;
    }

    // Get result 
    // TODO - format it 
    Matrix result = multiply(inverse(input_matrix), output_matrix);
    
    // Print result
    print_matrix(result);

    // Free dynamic array
    elementDynArray_free(&my_elements);
    return 0;
}