#include "circuit.h"
#include "matrix.h"

static int build_input_matrix(Matrix* input_matrix, Matrix conductance_matrix_G, Matrix incidence_matrix_B);
static int build_output_matrix(Matrix* output_matrix, Matrix current_vector_I, Matrix voltage_vector_E);

static int stamp(Element element, Matrix* G, Matrix* B, Matrix* I, Matrix* E, int* ivs_count);

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

        // Stamp that element and check for unknown element types
        if (stamp(current_element, &conductance_matrix_G, &incidence_matrix_B, &current_source_current_vector_I, &voltage_source_voltage_vector_E, &current_voltage_source_count)) {
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

// Stamp an element, add values to entries in matrices. ivs_count = independent voltage source count
int stamp(Element element, Matrix* G, Matrix* B, Matrix* I, Matrix* E, int* ivs_count) {
    int node_from = element.node_pos;
    int node_to = element.node_neg;

    // Check element type
    
    // Resistor
    if (element.type == 'R') {
        double resistance = element.value;

        // Build conductance matrix avoiding ground nodes
        if (node_from != 0) {
            G->data[node_from - 1][node_from - 1] += 1 / resistance;
        }
        if (node_to != 0) {
            G->data[node_to - 1][node_to - 1] += 1 / resistance;
        }

        if (node_from != 0 && node_to != 0) {
            G->data[node_from - 1][node_to - 1] -= 1 / resistance;
            G->data[node_to - 1][node_from - 1] -= 1 / resistance;
        }
    }

    // Independent current source
    else if (element.type == 'I') {
        double current = element.value;

        // Build current source current vector skipping ground
        if (node_from != 0) {
            I->data[node_from - 1][0] -= current;
        }
        if (node_to != 0) {
            I->data[node_to - 1][0] += current;
        }
    }

    // Independent voltage source
    else if (element.type == 'V') {
        double voltage = element.value;

        // Build voltage source voltage vector
        E->data[*ivs_count][0] = voltage;

        // Build incidence matrix but skip ground
        if (node_from != 0) {
            B->data[node_from - 1][*ivs_count] = +1;
        }
        if (node_to != 0) {
            B->data[node_to - 1][*ivs_count] = -1;
        }

        *ivs_count += 1;
    }

    // Unknown component
    else {
        return 1;
    }

    return 0;
}