#include "circuit.h"
#include "matrix.h"

static Matrix build_input_matrix(Matrix conductance_matrix_G, Matrix incidence_matrix_B);
static Matrix build_output_matrix(Matrix current_vector_I, Matrix voltage_vector_E);

int build_input_output_matrix(Matrix* input, Matrix* output, Circuit circuit) {
    ElementDynArray dynamic_element_array = circuit.elements;
    int node_number = circuit.node_number;
    int voltage_source_number = circuit.voltage_source_number;

    // Check for valid usage
    if (dynamic_element_array.element_array == NULL) {
        return 1;
    }
    else if (node_number == -1) {
        return 2;
    }
    else if (voltage_source_number == -1) {
        return 3;
    }
    else if (node_number - 1 + voltage_source_number > MAX_SIZE) {
        return 4;
    }

    // Input matrice constituents
    Matrix conductance_matrix_G = {.data = {{0}}, .rows = node_number - 1, .cols = node_number - 1};
    Matrix incidence_matrix_B = {.data = {{0}}, .rows = node_number - 1, .cols = voltage_source_number};

    // Output matrice constituents
    Matrix current_source_current_vector_I = {.data = {{0}}, .rows = node_number - 1, .cols = 1};
    Matrix voltage_source_voltage_vector_E = {.data = {{0}}, .rows = voltage_source_number, .cols = 1};

    int current_voltage_source_count = 0;

    // Loop through all elements
    for (int i = 0; i < dynamic_element_array.size; i++) {
        // Get element info
        Element current_element = dynamic_element_array.element_array[i];

        int node_from = current_element.node_pos;
        int node_to = current_element.node_neg;

        // Resistor
        if (current_element.type == 'R') {
            double resistance = current_element.value;

            // Build conductance matrix avoiding ground nodes
            if (node_from != 0) {
                conductance_matrix_G.data[node_from - 1][node_from - 1] += 1 / resistance;
            }
            if (node_to != 0) {
                conductance_matrix_G.data[node_to - 1][node_to - 1] += 1 / resistance;
            }

            if (node_from != 0 && node_to != 0) {
                conductance_matrix_G.data[node_from - 1][node_to - 1] -= 1 / resistance;
                conductance_matrix_G.data[node_to - 1][node_from - 1] -= 1 / resistance;
            }
        }
        // Independent voltage source
        else if (current_element.type == 'V') {
            double voltage = current_element.value;

            // Build voltage source voltage vector
            voltage_source_voltage_vector_E.data[current_voltage_source_count][0] = voltage;

            // Build incidence matrix but skip ground
            if (node_from != 0) {
                incidence_matrix_B.data[node_from - 1][current_voltage_source_count] = +1;
            }
            if (node_to != 0) {
                incidence_matrix_B.data[node_to - 1][current_voltage_source_count] = -1;
            }

            current_voltage_source_count += 1;
        }
        // Independent current source
        else if (current_element.type == 'I') {
            double current = current_element.value;

            // Build current source current vector skipping ground
            if (node_from != 0) {
                current_source_current_vector_I.data[node_from - 1][0] -= current;
            }
            if (node_to != 0) {
                current_source_current_vector_I.data[node_to - 1][0] += current;
            }
        }
        // Unknown component
        else {
            return 4;
        }
    }

    // Build input, output matrices
    *input = build_input_matrix(conductance_matrix_G, incidence_matrix_B);
    *output = build_output_matrix(current_source_current_vector_I, voltage_source_voltage_vector_E);

    return 0;
}

// Make complete input matrix = [G B]
//                              [C D]
Matrix build_input_matrix(Matrix conductance_matrix_G, Matrix incidence_matrix_B) {
    int rows = conductance_matrix_G.rows + incidence_matrix_B.cols;
    int cols = rows;

    // C = B transposed
    Matrix transpose_B = transpose(incidence_matrix_B);

    Matrix input_matrix = {.data = {{0}}, .rows = rows, .cols = cols};

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (i < conductance_matrix_G.rows) {
                // G
                if (j < conductance_matrix_G.cols) {
                    input_matrix.data[i][j] = conductance_matrix_G.data[i][j];
                }

                // B
                else {
                    input_matrix.data[i][j] = incidence_matrix_B.data[i][j - conductance_matrix_G.cols];
                }
            }
            else {
                // C
                if (j < conductance_matrix_G.cols) {
                    input_matrix.data[i][j] = transpose_B.data[i - conductance_matrix_G.rows][j];
                }
                // D = 0
                else {
                    input_matrix.data[i][j] = 0;
                }
            }
        }
    }

    return input_matrix;
}

// Make complete output matrix = [I]
//                               [E]
Matrix build_output_matrix(Matrix current_vector_I, Matrix voltage_vector_E) {
    int rows = current_vector_I.rows + voltage_vector_E.rows;
    int cols = 1;

    Matrix output_matrix = {.data = {{0}}, .rows = rows, .cols = cols};

    for (int i = 0; i < rows; i++) {
        // I
        if (i < current_vector_I.rows) {
            output_matrix.data[i][0] = current_vector_I.data[i][0];
        }
        // E
        else {
            output_matrix.data[i][0] = voltage_vector_E.data[i - current_vector_I.rows][0];
        }
    }

    return output_matrix;
}