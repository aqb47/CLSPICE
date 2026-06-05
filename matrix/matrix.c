#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "matrix.h"

static Matrix form_augmented_matrix(Matrix coefficient_matrix, Matrix result_matrix);
static void swap_greatest_row(Matrix* augmented_matrix, int target_row, int target_col);
static void add_row(Matrix* matrix, int row_to_be_added, int row_added_to, double row_to_add_scale, double row_add_to_scale);

// Initialize matrix to zero
Matrix Matrix_init(int rows, int cols) {
    // Bounds check
    if (rows < 0 || cols < 0) {
        printf("Invalid matrix row/ col initialization\n");
        return EMPTY_MATRIX;
    }

    // Initialize matrix
    Matrix new_matrix = {.rows = rows, .cols = cols};

    // Allocate memory for row pointers
    new_matrix.data = malloc(sizeof(double*) * rows);

    // Allocate memory for column pointers and initialize to zero
    for (int i = 0; i < rows; i++) {
        new_matrix.data[i] = calloc(cols, sizeof(double));
    }

    return new_matrix;
}

// Free matrix data
void Matrix_free(Matrix* matrix) {
    // Free column pointers
    for (int i = 0; i < matrix->rows; i++) {
        free(matrix->data[i]);
    }

    // Free row pointers
    free(matrix->data);

    // Set values to defaults
    matrix->data = NULL;
    matrix->rows = 0;
    matrix->cols = 0;
}

// Perform Gaussian Elimination to find variable values based on coefficient matrix and result vector
Matrix gaussian_elimination(Matrix coefficient_matrix, Matrix result_matrix) {
    // Check for correct usage
    if (coefficient_matrix.rows != coefficient_matrix.cols) {
        printf("Non-square matrix used as coefficient matrix during Gaussian Elimination\n");
        return EMPTY_MATRIX;
    }
    if (coefficient_matrix.rows != result_matrix.rows) {
        printf("Coefficient matrix rows and output vector rows do not match during Gaussian Elimination\n");
        return EMPTY_MATRIX;
    }
    if (result_matrix.cols != 1) {
        printf("Invalid column count for output vector, which must have a single column during Gaussian Elimination\n");
        return EMPTY_MATRIX;
    }

    // Augmented matrix that'll be coefficient matrix with another column representing result
    Matrix augmented_matrix = form_augmented_matrix(coefficient_matrix, result_matrix);
    if (augmented_matrix.data == NULL) {
        printf("Could not allocate memory for augmented matrix during Gaussian Elimination\n");
        return EMPTY_MATRIX;
    }

    // Pivot point will be on principal diagonal
    for (int i = 0; i < augmented_matrix.rows; i++) {
        // Perform partial pivoting
        swap_greatest_row(&augmented_matrix, i, i);

        // In case entry is zero the matrix is singular and has infinite/ zero solutions
        if (fabs(augmented_matrix.data[i][i]) < 1e-14) {
            return EMPTY_MATRIX;
        }

        // Make columns of rows below pivot zero
        for (int j = i + 1; j < augmented_matrix.rows; j++) {
            double scale = augmented_matrix.data[j][i] / augmented_matrix.data[i][i];
            add_row(&augmented_matrix, i, j, -scale, 1);
        }
    }

    // Row we start solving from will be at the bottom going to top for upper triangular matrices
    int starting_row = augmented_matrix.rows - 1;

    // Initialize result variable matrix with 0 as entry values and fill them up as we get solutions
    Matrix variable_matrix = Matrix_init(augmented_matrix.rows, 1);
    if (variable_matrix.data == NULL) {
        printf("Could not allocate memory for variable vector during Gaussian Elimination\n");

        Matrix_free(&augmented_matrix);
        return EMPTY_MATRIX;
    }

    // Move through augmented matrix bottom to top
    for (int i = starting_row; i >= 0; i--) {
        // Right hand side value
        double RHS = augmented_matrix.data[i][augmented_matrix.cols - 1];
        // Coefficient of variable
        double entry_coefficient = augmented_matrix.data[i][i];
        // The unknown variable
        double variable_value;

        // Subtract known variable values from RHS
        for (int j = augmented_matrix.rows - 1; j > i; j--) {
            RHS -= augmented_matrix.data[i][j] * variable_matrix.data[j][0];
        }

        // Calculate new variable value considering no zero division
        if (!(fabs(augmented_matrix.data[i][i]) < 1e-14)) {
            variable_value = RHS / entry_coefficient;
            variable_matrix.data[i][0] = variable_value;
        }
        else {
            printf("Singular coefficient matrix could not be solved during Gaussian Elimination\n");

            Matrix_free(&variable_matrix);
            Matrix_free(&augmented_matrix);

            return EMPTY_MATRIX;
        }
    }

    // Free allocated data for augmented matrix and return result
    Matrix_free(&augmented_matrix);
    return variable_matrix;
}

// Form augmented matrix for Gaussian elimination
Matrix form_augmented_matrix(Matrix coefficient_matrix, Matrix result_matrix) {
    int rows = coefficient_matrix.rows;
    int cols = coefficient_matrix.cols + result_matrix.cols;

    Matrix augmented_matrix_output = Matrix_init(rows, cols);
    if (augmented_matrix_output.data == NULL) {
        return EMPTY_MATRIX;
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            // First part of matrix horizontally is the coefficient matrix
            if (j < coefficient_matrix.cols) {
                augmented_matrix_output.data[i][j] = coefficient_matrix.data[i][j];
            }
            // Second part of matrix horizontally is the result matrix
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
    int max_row = target_row;

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

// Swap rows and columns of input matrix
Matrix transpose(Matrix input_matrix) {
    // Initialize output matrix
    Matrix output_matrix = Matrix_init(input_matrix.cols, input_matrix.rows);
    if (output_matrix.data == NULL) {
        return EMPTY_MATRIX;
    }

    // Go through every row
    for (int i = 0; i < output_matrix.rows; i++) {
        // Go through every column
        for (int j = 0; j < output_matrix.cols; j++) {
            // Swap entries 
            output_matrix.data[i][j] = input_matrix.data[j][i];
        }
    }

    return output_matrix;
}

// Print out every entry of a matrix
void print_matrix(Matrix A) {
    // Go through every rows
    for (int i = 0; i < A.rows; i++) {
        // Go through every column
        for (int j = 0; j < A.cols; j++) {
            // Print entry 
            printf("%lf ", A.data[i][j]);
        }

        printf("\n");
    }
}