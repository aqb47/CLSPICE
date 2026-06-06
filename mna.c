#include <string.h>
#include "circuit.h"
#include "matrix.h"

static void build_input_matrix(Matrix* input_matrix, Matrix conductance_matrix_G, Matrix incidence_matrix_B, Matrix incidence_matrix_C, Matrix control_matrix_D);
static void build_output_matrix(Matrix* output_matrix, Matrix current_vector_I, Matrix voltage_vector_E);

static void stamp_resistor(Element resistor, Matrix* G);

static void stamp_independent_current_source(Element current_source, Matrix* I);
static void stamp_independent_voltage_source(Element voltage_source, Matrix* B, Matrix* C, Matrix* E, int* ivs_counter);

static void stamp_VCVS(Element VCVS, Matrix* B, Matrix* C, Matrix* E, int* ivs_counter);
static void stamp_VCCS(Element VCCS, Matrix* G);
static void stamp_CCVS(Element CCVS, Matrix* B, Matrix* C, Matrix* D, Matrix* E, int* ivs_counter, Circuit circuit);
static void stamp_CCCS(Element CCCS, Matrix* B, Matrix* C, Circuit circuit);

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

        // Resistor
        if (current_element.type == 'R') {
            stamp_resistor(current_element, &conductance_matrix_G);
        }

        // Current source
        else if (current_element.type == 'I') {
            stamp_independent_current_source(current_element, &current_source_current_vector_I);
        }
        // Voltage source
        else if (current_element.type == 'V') {
            stamp_independent_voltage_source(current_element, &incidence_matrix_B, &incidence_matrix_C, &voltage_source_voltage_vector_E, &current_voltage_source_count);
        }

        // Voltage controlled voltage source
        else if (current_element.type == 'E') {
            stamp_VCVS(current_element, &incidence_matrix_B, &incidence_matrix_C, &voltage_source_voltage_vector_E, &current_voltage_source_count);
        }
        // Current controlled current source
        else if (current_element.type == 'F') {
            stamp_CCCS(current_element, &incidence_matrix_B, &incidence_matrix_C, circuit);
        }
        // Voltage controlled current source
        else if (current_element.type == 'G') {
            stamp_VCCS(current_element, &conductance_matrix_G);
        }
        // Current controlled voltage source
        else if (current_element.type == 'H') {
            stamp_CCVS(current_element, &incidence_matrix_B, &incidence_matrix_C, &control_matrix_D, &voltage_source_voltage_vector_E, &current_voltage_source_count, circuit);
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
void stamp_resistor(Element resistor, Matrix* G) {
    double resistance = resistor.value;
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
}

// Stamp an independent DC current source
void stamp_independent_current_source(Element current_source, Matrix* I) {
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
}

// Stamp an independent DC voltage source
void stamp_independent_voltage_source(Element voltage_source, Matrix* B, Matrix* C, Matrix* E, int* ivs_counter) {
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
}

// Stamp voltage controlled current source
void stamp_VCCS(Element VCCS, Matrix* G) {
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
}

// Stamp voltage controlled voltage source
void stamp_VCVS(Element VCVS, Matrix* B, Matrix* C, Matrix* E, int* ivs_counter) {
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
}

// Stamp current controlled voltage source
void stamp_CCVS(Element CCVS, Matrix* B, Matrix* C, Matrix* D, Matrix* E, int* ivs_counter, Circuit circuit) {
    double transresistance = CCVS.value;

    int node_pos = CCVS.node_pos;
    int node_neg = CCVS.node_neg;

    int control_vs_index = get_voltage_source_index(circuit, CCVS.ctrl_name);

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
}

// Stamp current controlled current source
void stamp_CCCS(Element CCCS, Matrix* B, Matrix* C, Circuit circuit) {
    double current_gain = CCCS.value;

    int node_pos = CCCS.node_pos;
    int node_neg = CCCS.node_neg;

    int control_vs_index = get_voltage_source_index(circuit, CCCS.ctrl_name);

    // Change B for non-grounded nodes of current source
    if (node_pos != 0) {
        B->data[node_pos - 1][control_vs_index] += current_gain;
        C->data[control_vs_index][node_pos - 1] += current_gain;
    }
    if (node_neg != 0) {
        B->data[node_neg - 1][control_vs_index] -= current_gain;
        C->data[control_vs_index][node_neg - 1] -= current_gain;
    }
}