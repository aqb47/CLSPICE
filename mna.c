#include "circuit.h"
#include "matrix.h"

static int build_input_matrix(Matrix* input_matrix, Matrix conductance_matrix_G, Matrix incidence_matrix_B);
static int build_output_matrix(Matrix* output_matrix, Matrix current_vector_I, Matrix voltage_vector_E);

static void stamp_resistor(Element resistor, Matrix* G);
static void stamp_independent_current_source(Element current_source, Matrix* I);
static void stamp_independent_voltage_source(Element voltage_source, Matrix* B, Matrix* E, int* ivs_counter);

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

    // Output matrice constituents
    Matrix current_source_current_vector_I = Matrix_init(node_number - 1, 1);
    Matrix voltage_source_voltage_vector_E = Matrix_init(voltage_source_number, 1);

    // Check if allocation was successful
    if (conductance_matrix_G.data == NULL || incidence_matrix_B.data == NULL || current_source_current_vector_I.data == NULL || voltage_source_voltage_vector_E.data == NULL) {
        printf("Could not allocate memory for G, B, I, E matrices during MNA\n");

        Matrix_free(&conductance_matrix_G);
        Matrix_free(&incidence_matrix_B);

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
            stamp_independent_voltage_source(current_element, &incidence_matrix_B, &voltage_source_voltage_vector_E, &current_voltage_source_count);
        }
        // Unknown component
        else {
            printf("Unknown component in netlist file\n");

            Matrix_free(&conductance_matrix_G);
            Matrix_free(&incidence_matrix_B);
            
            Matrix_free(&current_source_current_vector_I);
            Matrix_free(&voltage_source_voltage_vector_E);

            return 5;
        }
    }

    // Build input, output matrices
    build_input_matrix(input, conductance_matrix_G, incidence_matrix_B);
    build_output_matrix(output, current_source_current_vector_I, voltage_source_voltage_vector_E);

    // Free dynamically allocated matrices
    Matrix_free(&conductance_matrix_G);
    Matrix_free(&incidence_matrix_B);

    Matrix_free(&current_source_current_vector_I);
    Matrix_free(&voltage_source_voltage_vector_E);

    return 0;
}

// Make complete input matrix = [G B]
//                              [C D]
int build_input_matrix(Matrix* input_matrix, Matrix conductance_matrix_G, Matrix incidence_matrix_B) {
    int rows = conductance_matrix_G.rows + incidence_matrix_B.cols;
    int cols = rows;

    // C = B transposed
    Matrix transpose_B = transpose(incidence_matrix_B);

    if (transpose_B.data == NULL) {
        printf("Could not allocate memory for C\n");

        return 1;
    }

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
                    input_matrix->data[i][j] = transpose_B.data[i - conductance_matrix_G.rows][j];
                }
                // D = 0
                else {
                    input_matrix->data[i][j] = 0;
                }
            }
        }
    }

    // Free dynamically allocated temporary matrix and return input matrix
    Matrix_free(&transpose_B);
    return 0;
}

// Make complete output matrix = [I]
//                               [E]
int build_output_matrix(Matrix* output_matrix, Matrix current_vector_I, Matrix voltage_vector_E) {
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

    return 0;
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
void stamp_independent_voltage_source(Element voltage_source, Matrix* B, Matrix* E, int* ivs_counter) {
    double voltage = voltage_source.value;
    int node_pos = voltage_source.node_pos;
    int node_neg = voltage_source.node_neg;

    // +1 for non-grounded node connected to positive terminal of voltage source
    if (node_pos != 0) {
        B->data[node_pos - 1][*ivs_counter] = +1;
    }
    // -1 for non-grounded node connected to negative terminal of voltage source
    if (node_neg != 0) {
        B->data[node_neg - 1][*ivs_counter] = -1;
    }

    // Add voltage to E
    E->data[*ivs_counter][0] = voltage;

    // Increment independent voltage source counter
    *ivs_counter += 1;
}
