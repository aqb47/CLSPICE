#include "circuit.h"
#include "parser.h"

int main(void) {
    char* netlist_filename = "netlist.sp";

    ElementDynArray my_elements = elementDynArray_init(DEFAULT_CAPACITY);

    if (parse_file(netlist_filename, &my_elements)) {
        return 1;
    }

    int node_number = get_node_number(my_elements);
    int voltage_source_number = get_voltage_source_number(my_elements);

    Circuit my_circuit = {
        .elements = my_elements,
        .node_number = node_number,
        .voltage_source_number = voltage_source_number
    };

    // Do stuff
    print_circuit(my_circuit);

    elementDynArray_free(&my_elements);
    return 0;
}