#include <stdio.h>
#include <math.h>
#include "matrix.h"

static Matrix negate(Matrix A);
static Matrix generate_submatrix(Matrix A, int row, int col);
static Matrix multiply_constant(double constant, Matrix A);

static Matrix form_augmented_matrix(Matrix coefficient_matrix, Matrix result_matrix);
static void swap_greatest_row(Matrix* augmented_matrix, int target_row, int target_col);
static void add_row(Matrix* matrix, int row_to_be_added, int row_added_to, double row_to_add_scale, double row_add_to_scale);

// Add two matrices A and B with same dimensions
Matrix add(Matrix A, Matrix B) {
    // If dimensions don't match return an empty matrix
    if (A.rows != B.rows || A.cols != B.cols) {
        return EMPTY_MATRIX; 
    }

    // Initialize result matrix C
    int rows = A.rows;
    int cols = A.cols;

    Matrix C = {.data = {{0}}, .rows = rows, .cols = cols};

    // For each row
    for (int i = 0; i < rows; i++) {
        // For each column in row
        for (int j = 0; j < cols; j++) {
            // Add entries from A and B into C
            C.data[i][j] = A.data[i][j] + B.data[i][j];
        }
    }

    return C;
}

// Negate all elements of A
Matrix negate(Matrix A) {
    int rows = A.rows;
    int cols = A.cols;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            A.data[i][j] = -A.data[i][j];
        }
    }

    return A;
}

// Subtract B from A
Matrix subtract(Matrix A, Matrix B) {
    B = negate(B);

    return add(A, B);
}

// Multiply A and B in order A * B
Matrix multiply(Matrix A, Matrix B) {
    int A_rows = A.rows;
    int A_cols = A.cols;

    int B_rows = B.rows;
    int B_cols = B.cols;

    // Return an empty matrix if dimensions do not match
    if (A_cols != B_rows) {
        return EMPTY_MATRIX;
    }

    // Initialize result matrix C with dimensions A_rows x B_cols
    Matrix C = {.data = {{0}}, .rows = A_rows, .cols = B_cols};

    for (int i = 0; i < A_rows; i++) {
        for (int j = 0; j < B_cols; j++) {
            for (int k = 0; k < A_cols; k++) {
                C.data[i][j] += A.data[i][k] * B.data[k][j];
            }
        }
    }

    return C;
}

// Swap rows and columns of a matrix
Matrix transpose(Matrix A) {
    int rows = A.rows;
    int cols = A.cols;

    // Initialize result matrix B with swapped dimensions
    Matrix B = {{{0}}, cols, rows};

    // Go through each row
    for (int i = 0; i < rows; i++) {
        // Go throuch each column
        for (int j = 0; j < cols; j++) {
            // Swap [row][col] and [col][row]
            B.data[j][i] = A.data[i][j];
        }
    }

    return B;
}

// Get determinant from a matrix. Returns NAN if something goes wrong
double determinant(Matrix A) {
    int rows = A.rows;
    int cols = A.cols;

    // n represents rows or cols of square matrix
    int n = rows;

    double det_value = 0;

    // Non-square matrices do not have a determinant
    if (rows != cols) {
        return NAN;
    }

    // 1x1 matrices determinant is their first value
    if (n == 1) {
        det_value = A.data[0][0];
    }

    // 2x2 matrices e.g [a b]
    //                  [c d]
    // have a determinant of ad - bc
    else if (n == 2) {
        det_value = A.data[0][0] * A.data[1][1] - A.data[0][1] * A.data[1][0];
    }

    // Recursively generate determinant for n > 2
    else {
        for (int i = 0; i < cols; i++) {
            Matrix submatrix = generate_submatrix(A, 0, i);
            int sign = (i % 2 == 0) ? 1: -1;

            det_value += sign * A.data[0][i] * determinant(submatrix);
        }
    }

    return det_value;
}

// Generate submatrix with respect to row, col position (which is zero-based)
Matrix generate_submatrix(Matrix A, int row, int col) {
    // Initialize submatrix which will have one less row and one less col than initial square matrix
    int A_rows = A.rows;
    int A_cols = A.cols;

    Matrix B = {{{0}}, A_rows - 1, A_cols - 1};

    // For nth entry of submatrix count represents n - 1. This will be used to generate position of each entry of submatrix
    int count = 0;

    // Go throuch each row of initial matrix
    for (int i = 0; i < A_rows; i++) {
        // Go throuch each column of initial matrix
        for (int j = 0; j < A_cols; j++) {
            // Skip rows or columns from position which w.r.t we're finding the submatrix
            if (i == row || j == col) {
                continue;
            }

            // Generate submatrix position
            int submatrix_row = floor(count / (A_rows - 1));
            int submatrix_col = count % (A_rows - 1);

            // Assign values
            B.data[submatrix_row][submatrix_col] = A.data[i][j];
            count++;
        }
    }

    return B;
}

// Multiply a matrix with a constant. This returns a matrix with every entry multiplied by that constant
Matrix multiply_constant(double constant, Matrix A) {
    int rows = A.rows;
    int cols = A.cols;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            A.data[i][j] *= constant;
        }
    }

    return A;
}

// Get an inverse matrix (matrix that multiplied by input produces an identity matrix)
Matrix inverse(Matrix A) {
    int rows = A.rows;
    int cols = A.cols;

    // Non-square matrices do not have an inverse matrix
    if (rows != cols) {
        return EMPTY_MATRIX;
    }

    double det_value = determinant(A);

    // A matrix with zero as a determinant also does not have an inverse matrix
    if (fabs(det_value) < 1e-9) {
        return EMPTY_MATRIX;
    }

    // For a 1x1 matrix
    if (rows == 1) {
        Matrix inv_A = {.rows = 1, .cols = 1};
        inv_A.data[0][0] = 1 / A.data[0][0];

        return inv_A;
    }

    // Initialize cofactor matrix
    Matrix cofactor_A = {{{0}}, rows, cols};

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int sign = ((i + j) % 2 == 0) ? 1 : -1;

            cofactor_A.data[i][j] = sign * determinant(generate_submatrix(A, i, j));
        }
    }

    // Calculate inverse matrix
    Matrix inv_A = multiply_constant(1 / det_value, transpose(cofactor_A));

    return inv_A;
}

Matrix gaussian_elimination(Matrix coefficient_matrix, Matrix result_matrix) {
    // Check for correct usage
    if (coefficient_matrix.rows != coefficient_matrix.cols) {
        return EMPTY_MATRIX;
    }
    if (coefficient_matrix.rows != result_matrix.rows) {
        return EMPTY_MATRIX;
    }
    if (result_matrix.cols != 1) {
        return EMPTY_MATRIX;
    }

    // Augmented matrix that'll be coefficient matrix with another column representing result
    Matrix augmented_matrix = form_augmented_matrix(coefficient_matrix, result_matrix);

    // Pivot point will be on principal diagonal
    for (int i = 0; i < augmented_matrix.rows; i++) {
        // Perform partial pivoting
        swap_greatest_row(&augmented_matrix, i, i);

        // In case entry is zero 
        if (fabs(augmented_matrix.data[i][i]) < 1e-9) {
            return EMPTY_MATRIX;
        }

        // Make columns of rows below pivot zero
        for (int j = i + 1; j < augmented_matrix.rows; j++) {
            double scale = augmented_matrix.data[j][i] / augmented_matrix.data[i][i];
            add_row(&augmented_matrix, i, j, -scale, 1);
        }
    }

    // This'll count down as we solve the augmented matrix (zero-based)
    int variable_to_solve = coefficient_matrix.cols - 1;
    int total_variables = variable_to_solve;

    // Row we start solving from will be at the bottom going to top for upper triangular matrices
    int starting_row = augmented_matrix.rows - 1;

    // Initialize result variable matrix with 0 as entry values and fill them up as we get solutions
    Matrix variable_matrix = {.rows = augmented_matrix.rows, .cols = 1, .data = {{0}}};

    // Move through augmented matrix bottom to top
    for (int i = starting_row; i >= 0; i--) {
        // Right hand side value
        double RHS = augmented_matrix.data[i][augmented_matrix.cols - 1];
        // Coefficient of variable
        double entry_coefficient = augmented_matrix.data[i][variable_to_solve];
        // The unknown variable
        double variable_value;

        // Subtract known variable values from RHS
        for (int j = total_variables; j > variable_to_solve; j--) {
            RHS -= augmented_matrix.data[i][j] * variable_matrix.data[j][0];
        }

        // Calculate new variable value considering no zero division
        if (!(fabs(augmented_matrix.data[i][variable_to_solve]) < 1e-9)) {
            variable_value = RHS / entry_coefficient;
            variable_matrix.data[variable_to_solve][0] = variable_value;
        }
        else {
            return EMPTY_MATRIX;
        }

        // Move on to next variable
        variable_to_solve -= 1;
    }

    return variable_matrix;
}

// Form augmented matrix for Gaussian elimination
Matrix form_augmented_matrix(Matrix coefficient_matrix, Matrix result_matrix) {
    int rows = coefficient_matrix.rows;
    int cols = coefficient_matrix.cols + result_matrix.cols;

    Matrix augmented_matrix_output = {.rows = rows, .cols = cols, .data = {{0}}};

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (j < coefficient_matrix.cols) {
                augmented_matrix_output.data[i][j] = coefficient_matrix.data[i][j];
            }
            else {
                augmented_matrix_output.data[i][j] = result_matrix.data[i][j - coefficient_matrix.cols];
            }
        }
    }

    return augmented_matrix_output;
}

// Check for row with greatest value in target column, and swap it to target row
void swap_greatest_row(Matrix* augmented_matrix, int target_row, int target_col) {
    double max = 0;
    int max_row = 0;

    // Get maximum value and position of row with maximum value
    for (int i = target_row; i < augmented_matrix->rows; i++) {
        if (fabs(augmented_matrix->data[i][target_col]) > max) {
            max = fabs(augmented_matrix->data[i][target_col]);
            max_row = i;
        }
    }

    // If not, swap largest entry row to target row
    for (int j = 0; j < augmented_matrix->cols; j++) {
        double temp = augmented_matrix->data[max_row][j];

        augmented_matrix->data[max_row][j] = augmented_matrix->data[target_row][j];
        augmented_matrix->data[target_row][j] = temp;
    }

    return;
}

// Add one row to another 
void add_row(Matrix* matrix, int row_to_be_added, int row_added_to, double row_to_add_scale, double row_add_to_scale) {
    for (int i = 0; i < matrix->cols; i++) {
        matrix->data[row_added_to][i] = (matrix->data[row_to_be_added][i]*row_to_add_scale) + (matrix->data[row_added_to][i]*row_add_to_scale);
    }
}