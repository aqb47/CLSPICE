#include <string.h>
#include "circuit.h"
#include "matrix.h"

static void build_input_matrix(Matrix* input_matrix, Matrix conductance_matrix_G, Matrix incidence_matrix_B, Matrix incidence_matrix_C, Matrix control_matrix_D);
static void build_output_matrix(Matrix* output_matrix, Matrix current_vector_I, Matrix voltage_vector_E);

static int stamp_resistor(Element resistor, Matrix* G);

static int stamp_independent_current_source(Element current_source, Matrix* I);
static int stamp_independent_voltage_source(Element voltage_source, Matrix* B, Matrix* C, Matrix* E, int* ivs_counter);

static int stamp_VCVS(Element VCVS, Matrix* B, Matrix* C, Matrix* E, int* ivs_counter);
static int stamp_VCCS(Element VCCS, Matrix* G);
static int stamp_CCVS(Element CCVS, Matrix* B, Matrix* C, Matrix* D, Matrix* E, int* ivs_counter, Circuit circuit);
static int stamp_CCCS(Element CCCS, Matrix* B, Circuit circuit);

int build_input_output_matrix(Matrix* input, Matrix* output, Circuit circuit) {
    ElementDynArray dynamic_element_array = circuit.elements;
    int node_number = circuit.node_number;
    int voltage_source_number = circuit.voltage_source_number;

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

    // Input matrice constituents
    Matrix conductance_matrix_G = Matrix_init(node_number - 1, node_number - 1);
    Matrix incidence_matrix_B = Matrix_init(node_number - 1, voltage_source_number);
    Matrix incidence_matrix_C = Matrix_init(voltage_source_number, node_number - 1);
    Matrix control_matrix_D = Matrix_init(voltage_source_number, voltage_source_number);

    // Output matrice constituents
    Matrix current_source_current_vector_I = Matrix_init(node_number - 1, 1);
    Matrix voltage_source_voltage_vector_E = Matrix_init(voltage_source_number, 1);

    // Check if allocation was successful
    if (conductance_matrix_G.data == NULL || incidence_matrix_B.data == NULL || incidence_matrix_C.data == NULL || current_source_current_vector_I.data == NULL || voltage_source_voltage_vector_E.data == NULL) {
        printf("Could not allocate memory for G, B, I, E matrices during MNA\n");

        Matrix_free(&conductance_matrix_G);
        Matrix_free(&incidence_matrix_B);
        Matrix_free(&incidence_matrix_C);
        Matrix_free(&control_matrix_D);

        Matrix_free(&current_source_current_vector_I);
        Matrix_free(&voltage_source_voltage_vector_E);

        return 4;
    }

    int current_voltage_source_count = 0;

    // Loop through all elements
    for (int i = 0; i < dynamic_element_array.size; i++) {
        // Get element info
        Element current_element = dynamic_element_array.element_array[i];
        int return_code;

        // Resistor
        if (current_element.type == 'R') {
            return_code = stamp_resistor(current_element, &conductance_matrix_G);
        }

        // Current source
        else if (current_element.type == 'I') {
            return_code = stamp_independent_current_source(current_element, &current_source_current_vector_I);
        }
        // Voltage source
        else if (current_element.type == 'V') {
            return_code = stamp_independent_voltage_source(current_element, &incidence_matrix_B, &incidence_matrix_C, &voltage_source_voltage_vector_E, &current_voltage_source_count);
        }

        // Voltage controlled voltage source
        else if (current_element.type == 'E') {
            return_code = stamp_VCVS(current_element, &incidence_matrix_B, &incidence_matrix_C, &voltage_source_voltage_vector_E, &current_voltage_source_count);
        }
        // Current controlled current source - can go wrong if control source doesn't exist
        else if (current_element.type == 'F') {
            return_code = stamp_CCCS(current_element, &incidence_matrix_B, circuit);
        }
        // Voltage controlled current source
        else if (current_element.type == 'G') {
            return_code = stamp_VCCS(current_element, &conductance_matrix_G);
        }
        // Current controlled voltage source - can go wrong if control source doesn't exist
        else if (current_element.type == 'H') {
            return_code = stamp_CCVS(current_element, &incidence_matrix_B, &incidence_matrix_C, &control_matrix_D, &voltage_source_voltage_vector_E, &current_voltage_source_count, circuit);
        }

        // Unknown component
        else {
            printf("Unknown component in netlist file\n");

            Matrix_free(&conductance_matrix_G);
            Matrix_free(&incidence_matrix_B);
            Matrix_free(&incidence_matrix_C);
            
            Matrix_free(&current_source_current_vector_I);
            Matrix_free(&voltage_source_voltage_vector_E);

            return 5;
        }

        // Check if stamping was successful
        if (return_code) {
            printf("Stamping was unsuccessful for element\n");

            Matrix_free(&conductance_matrix_G);
            Matrix_free(&incidence_matrix_B);
            Matrix_free(&incidence_matrix_C);
            
            Matrix_free(&current_source_current_vector_I);
            Matrix_free(&voltage_source_voltage_vector_E);

            return 6;
        }
    }

    // Build input, output matrices 
    build_input_matrix(input, conductance_matrix_G, incidence_matrix_B, incidence_matrix_C, control_matrix_D);
    build_output_matrix(output, current_source_current_vector_I, voltage_source_voltage_vector_E);

    // Free dynamically allocated matrices
    Matrix_free(&conductance_matrix_G);
    Matrix_free(&incidence_matrix_B);
    Matrix_free(&incidence_matrix_C);
    Matrix_free(&control_matrix_D);

    Matrix_free(&current_source_current_vector_I);
    Matrix_free(&voltage_source_voltage_vector_E);

    return 0;
}

// Make complete input matrix = [G B]
//                              [C D]
void build_input_matrix(Matrix* input_matrix, Matrix conductance_matrix_G, Matrix incidence_matrix_B, Matrix incidence_matrix_C, Matrix control_matrix_D) {
    int rows = conductance_matrix_G.rows + incidence_matrix_B.cols;
    int cols = rows;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (i < conductance_matrix_G.rows) {
                // G
                if (j < conductance_matrix_G.cols) {
                    input_matrix->data[i][j] = conductance_matrix_G.data[i][j];
                }

                // B
                else {
                    input_matrix->data[i][j] = incidence_matrix_B.data[i][j - conductance_matrix_G.cols];
                }
            }
            else {
                // C
                if (j < conductance_matrix_G.cols) {
                    input_matrix->data[i][j] = incidence_matrix_C.data[i - conductance_matrix_G.rows][j];
                }
                // D = 0
                else {
                    input_matrix->data[i][j] = control_matrix_D.data[i - conductance_matrix_G.rows][j - conductance_matrix_G.cols];
                }
            }
        }
    }
}

// Make complete output matrix = [I]
//                               [E]
void build_output_matrix(Matrix* output_matrix, Matrix current_vector_I, Matrix voltage_vector_E) {
    int rows = current_vector_I.rows + voltage_vector_E.rows;

    for (int i = 0; i < rows; i++) {
        // I
        if (i < current_vector_I.rows) {
            output_matrix->data[i][0] = current_vector_I.data[i][0];
        }
        // E
        else {
            output_matrix->data[i][0] = voltage_vector_E.data[i - current_vector_I.rows][0];
        }
    }
}

// Stamp a DC fixed resistance resistor
int stamp_resistor(Element resistor, Matrix* G) {
    if (resistor.node_pos < 0 || resistor.node_neg < 0) {
        printf("Invalid node number for %s\n", resistor.name);
        return 1;
    }

    double resistance = resistor.value;
    if (resistance < 0 || fabs(resistance) < 1e-14) {
        printf("Invalid resistance value for %s\n", resistor.name);
        return 2;
    }

    int node_pos = resistor.node_pos;
    int node_neg = resistor.node_neg;
    
    // Add positive conductance to non-grounded nodes on-diagonal
    if (node_pos != 0) {
        G->data[node_pos - 1][node_pos - 1] += 1 / resistance;
    }
    if (node_neg != 0) {
        G->data[node_neg - 1][node_neg - 1] += 1 / resistance;
    }

    // Add negative conductances between non-grounded nodes off-diagonal
    if (node_pos != 0 && node_neg != 0) {
        G->data[node_pos - 1][node_neg - 1] -= 1 / resistance;
        G->data[node_neg - 1][node_pos - 1] -= 1 / resistance;
    }

    return 0;
}

// Stamp an independent DC current source
int stamp_independent_current_source(Element current_source, Matrix* I) {
    if (current_source.node_pos < 0 || current_source.node_neg < 0) {
        printf("Invalid node number for %s\n", current_source.name);
        return 1;
    }

    double current = current_source.value;
    int node_pos = current_source.node_pos;
    int node_neg = current_source.node_neg;

    // Decrement current source from non-grounded positive node position
    if (node_pos != 0) {
        I->data[node_pos - 1][0] -= current;
    }
    // Increment current source to non-grounded negative node position
    if (node_neg != 0) {
        I->data[node_neg - 1][0] += current;
    }

    return 0;
}

// Stamp an independent DC voltage source
int stamp_independent_voltage_source(Element voltage_source, Matrix* B, Matrix* C, Matrix* E, int* ivs_counter) {
    if (voltage_source.node_pos < 0 || voltage_source.node_neg < 0) {
        printf("Invalid node number for %s\n", voltage_source.name);
        return 1;
    }

    double voltage = voltage_source.value;
    int node_pos = voltage_source.node_pos;
    int node_neg = voltage_source.node_neg;

    // +1 for non-grounded node connected to positive terminal of voltage source
    if (node_pos != 0) {
        B->data[node_pos - 1][*ivs_counter] = +1;
        C->data[*ivs_counter][node_pos - 1] = +1;
    }
    // -1 for non-grounded node connected to negative terminal of voltage source
    if (node_neg != 0) {
        B->data[node_neg - 1][*ivs_counter] = -1;
        C->data[*ivs_counter][node_neg - 1] = -1;
    }

    // Add voltage to E
    E->data[*ivs_counter][0] = voltage;

    // Increment voltage source counter
    *ivs_counter += 1;

    return 0;
}

// Stamp voltage controlled current source
int stamp_VCCS(Element VCCS, Matrix* G) {
    if (VCCS.node_pos < 0 || VCCS.node_neg < 0) {
        printf("Invalid node number for %s\n", VCCS.name);
        return 1;
    }  
    else if (VCCS.ctrl_node_pos == NON_VC_ELEMENT_NODE || VCCS.ctrl_node_neg == NON_VC_ELEMENT_NODE) {
        printf("Invalid control node number for %s\n", VCCS.name);
        return 2;
    }

    double transconductance = VCCS.value;

    int node_pos = VCCS.node_pos;
    int node_neg = VCCS.node_neg;

    int ctrl_node_pos = VCCS.ctrl_node_pos;
    int ctrl_node_neg = VCCS.ctrl_node_neg;

    // Increment entries in conductance matrix based on non-grounded nodes
    if (node_pos != 0 && ctrl_node_pos != 0) {
        G->data[node_pos - 1][ctrl_node_pos - 1] += transconductance;
    }

    if (node_pos != 0 && ctrl_node_neg != 0) {
        G->data[node_pos - 1][ctrl_node_neg - 1] -= transconductance;
    }

    if (node_neg != 0 && ctrl_node_pos != 0) {
        G->data[node_neg - 1][ctrl_node_pos - 1] -= transconductance;
    }

    if (node_neg != 0 && ctrl_node_neg != 0) {
        G->data[node_neg - 1][ctrl_node_neg - 1] += transconductance;
    }

    return 0;
}

// Stamp voltage controlled voltage source
int stamp_VCVS(Element VCVS, Matrix* B, Matrix* C, Matrix* E, int* ivs_counter) {
    if (VCVS.node_pos < 0 || VCVS.node_neg < 0) {
        printf("Invalid node number for %s\n", VCVS.name);
        return 1;
    }
    else if (VCVS.ctrl_node_pos == NON_VC_ELEMENT_NODE || VCVS.ctrl_node_neg == NON_VC_ELEMENT_NODE) {
        printf("Invalid control node number for %s\n", VCVS.name);
        return 2;
    }

    double voltage_gain = VCVS.value;

    int node_pos = VCVS.node_pos;
    int node_neg = VCVS.node_neg;

    int ctrl_node_pos = VCVS.ctrl_node_pos;
    int ctrl_node_neg = VCVS.ctrl_node_neg;

    // +1 for non-grounded node connected to positive terminal of voltage source
    if (node_pos != 0) {
        B->data[node_pos - 1][*ivs_counter] = +1;
        C->data[*ivs_counter][node_pos - 1] = +1;
    }
    // -1 for non-grounded node connected to negative terminal of voltage source
    if (node_neg != 0) {
        B->data[node_neg - 1][*ivs_counter] = -1;
        C->data[*ivs_counter][node_neg - 1] = -1;
    }

    // Add dependency to C
    if (ctrl_node_pos != 0) {
        C->data[*ivs_counter][ctrl_node_pos - 1] -= voltage_gain;
    }
    if (ctrl_node_neg != 0) {
        C->data[*ivs_counter][ctrl_node_neg - 1] += voltage_gain;
    }

    // Change E
    E->data[*ivs_counter][0] = 0;

    // Increment counter
    *ivs_counter += 1;

    return 0;
}

// Stamp current controlled voltage source
int stamp_CCVS(Element CCVS, Matrix* B, Matrix* C, Matrix* D, Matrix* E, int* ivs_counter, Circuit circuit) {
    if (CCVS.node_pos < 0 || CCVS.node_neg < 0) {
        printf("Invalid node number for %s\n", CCVS.name);
        return 1;
    }

    double transresistance = CCVS.value;

    int node_pos = CCVS.node_pos;
    int node_neg = CCVS.node_neg;

    int control_vs_index = get_voltage_source_index(circuit, CCVS.ctrl_name);
    if (control_vs_index == VS_NOT_FOUND) {
        printf("Control voltage source (%s) for CCVS (%s) not found\n", CCVS.ctrl_name, CCVS.name);
        return 2;
    }

    // +1 for non-grounded node connected to positive terminal of voltage source
    if (node_pos != 0) {
        B->data[node_pos - 1][*ivs_counter] = +1;
        C->data[*ivs_counter][node_pos - 1] = +1;
    }
    // -1 for non-grounded node connected to negative terminal of voltage source
    if (node_neg != 0) {
        B->data[node_neg - 1][*ivs_counter] = -1;
        C->data[*ivs_counter][node_neg - 1] = -1;
    }

    // Decrement transresistance from D
    D->data[*ivs_counter][control_vs_index] -= transresistance;

    // Change E
    E->data[*ivs_counter][0] = 0;
    
    // Increment counter
    *ivs_counter += 1;

    return 0;
}

// Stamp current controlled current source
int stamp_CCCS(Element CCCS, Matrix* B, Circuit circuit) {
    if (CCCS.node_pos < 0 || CCCS.node_neg < 0) {
        printf("Invalid node number for %s\n", CCCS.name);
        return 1;
    }

    double current_gain = CCCS.value;

    int node_pos = CCCS.node_pos;
    int node_neg = CCCS.node_neg;

    int control_vs_index = get_voltage_source_index(circuit, CCCS.ctrl_name);
    if (control_vs_index == VS_NOT_FOUND) {
        printf("Control voltage source (%s) for CCCS (%s) not found\n", CCCS.ctrl_name, CCCS.name);
        return 2;
    }

    // Change B for non-grounded nodes of current source
    if (node_pos != 0) {
        B->data[node_pos - 1][control_vs_index] += current_gain;
    }
    if (node_neg != 0) {
        B->data[node_neg - 1][control_vs_index] -= current_gain;
    }

    return 0;
}