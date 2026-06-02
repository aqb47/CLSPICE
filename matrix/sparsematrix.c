#include "sparsematrix.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static void SparseMatrix_COO_sort(SparseMatrix_COO* sparse_matrix);
static int SparseMatrix_CSR_add_entry(SparseMatrix_CSR* sparse_matrix, int entry_row, int entry_col, double value, int first_entry_of_row);

SparseMatrix_COO SparseMatrix_COO_init(int rows, int cols) {
    SparseMatrix_COO sparse_matrix = {
        .rows = rows,
        .cols = cols,
        .number_of_nonzeroes = 0,

        // Dynamic stuff
        .nonzero_col_indices = NULL,
        .nonzero_row_indices = NULL,
        .values = NULL
    };

    return sparse_matrix;
}

SparseMatrix_CSR SparseMatrix_CSR_init(int rows, int cols) {
    SparseMatrix_CSR sparse_matrix = {
        .rows = rows,
        .cols = cols,
        .number_of_nonzeroes = 0,

        // Dynamic stuff
        // The row pointer always has rows number of elements and can be malloc'd right here
        .row_ptr = malloc(sizeof(int) * (rows + 1)),
        .nonzero_col_indices = NULL,
        .values = NULL
    };

    return sparse_matrix;
}

int SparseMatrix_COO_add_entry(SparseMatrix_COO* sparse_matrix, int entry_row, int entry_col, double value) {
    // Can't add zero valued entry
    if (value == 0) {
        return 1;
    }

    // Bounds check
    if (entry_row + 1 > sparse_matrix->rows || entry_col + 1 > sparse_matrix->cols) {
        return 2;
    }

    // Check for duplicate entry
    for (int i = 0; i < sparse_matrix->number_of_nonzeroes; i++) {
        if (sparse_matrix->nonzero_row_indices[i] == entry_row && sparse_matrix->nonzero_col_indices[i] == entry_col) {
            sparse_matrix->values[i] += value;

            return 0;
        }
    }

    // Reallocate values, row and column index arrays
    double* temp_values = realloc(sparse_matrix->values, sizeof(double) * (sparse_matrix->number_of_nonzeroes + 1));
    int* temp_row = realloc(sparse_matrix->nonzero_row_indices, sizeof(int) * (sparse_matrix->number_of_nonzeroes + 1));
    int* temp_col = realloc(sparse_matrix->nonzero_col_indices, sizeof(int) * (sparse_matrix->number_of_nonzeroes + 1));

    // If reallocation goes wrong
    if (temp_values == NULL || temp_row == NULL || temp_col == NULL) {
        free(temp_values);
        free(temp_row);
        free(temp_col);

        return 3;
    }

    // Have pointers point to successfuly reallocated memory
    sparse_matrix->values = temp_values;
    sparse_matrix->nonzero_row_indices = temp_row;
    sparse_matrix->nonzero_col_indices = temp_col;

    int index_to_be_added = sparse_matrix->number_of_nonzeroes;

    // Append new entry info
    sparse_matrix->values[index_to_be_added] = value;
    sparse_matrix->nonzero_row_indices[index_to_be_added] = entry_row;
    sparse_matrix->nonzero_col_indices[index_to_be_added] = entry_col;
    
    // Increment non-zero count
    sparse_matrix->number_of_nonzeroes += 1;

    return 0;
}

// This should only be used for sorted row, col entries
int SparseMatrix_CSR_add_entry(SparseMatrix_CSR* sparse_matrix, int entry_row, int entry_col, double value, int first_entry_of_row) {
    // Can't add zero valued entry
    if (value == 0) {
        return 1;
    }

    // Bounds check
    if (entry_row + 1 > sparse_matrix->rows || entry_col + 1 > sparse_matrix->cols) {
        return 2;
    }

    // Reallocate cols index, row pointer and value arrays
    double* temp_values = realloc(sparse_matrix->values, sizeof(double) * (sparse_matrix->number_of_nonzeroes + 1));
    int* temp_col = realloc(sparse_matrix->nonzero_col_indices, sizeof(int) * (sparse_matrix->number_of_nonzeroes + 1));
    
    // If reallocation goes wrong
    if (temp_values == NULL || temp_col == NULL) {
        free(temp_values);
        free(temp_col);

        return 3;
    }

    // Have pointers point to successfuly reallocated memory
    sparse_matrix->values = temp_values;
    sparse_matrix->nonzero_col_indices = temp_col;

    int index_to_be_added = sparse_matrix->number_of_nonzeroes;

    // Append new entry info
    sparse_matrix->values[index_to_be_added] = value;
    sparse_matrix->nonzero_col_indices[index_to_be_added] = entry_col;

    if (first_entry_of_row) {
        for (int i = entry_row; i < sparse_matrix->rows + 1; i++) {
            sparse_matrix->row_ptr[i] = index_to_be_added;
        }
    }

    // Increment non-zero count
    sparse_matrix->number_of_nonzeroes += 1;

    return 0;
}

void SparseMatrix_COO_free(SparseMatrix_COO* sparse_matrix) {
    // Free dynamically allocated values 
    free(sparse_matrix->values);
    free(sparse_matrix->nonzero_row_indices);
    free(sparse_matrix->nonzero_col_indices);

    // Reset data
    sparse_matrix->values = NULL;
    sparse_matrix->nonzero_col_indices = NULL;
    sparse_matrix->nonzero_row_indices = NULL;
}

void SparseMatrix_CSR_free(SparseMatrix_CSR* sparse_matrix) {
    // Free dynamically allocated values 
    free(sparse_matrix->row_ptr);
    free(sparse_matrix->nonzero_col_indices);
    free(sparse_matrix->values);

    // Reset data
    sparse_matrix->values = NULL;
    sparse_matrix->nonzero_col_indices = NULL;
    sparse_matrix->row_ptr = NULL;
}

double SparseMatrix_COO_get_entry(const SparseMatrix_COO* sparse_matrix, int entry_row, int entry_col) {
    // Bounds check
    if (entry_row + 1 > sparse_matrix->rows || entry_col + 1 > sparse_matrix->cols) {
        return NAN;
    }

    // Loop over non-zero entries
    for (int i = 0; i < sparse_matrix->number_of_nonzeroes; i++) {
        // Check if row, col indices of non-zero entry match input row, col indices
        if (sparse_matrix->nonzero_row_indices[i] == entry_row && sparse_matrix->nonzero_col_indices[i] == entry_col) {
            // We found non-zero entry
            return sparse_matrix->values[i];
        }
    }

    // If no non-zero entry the entry has to be zero
    return 0.0;
}

SparseMatrix_CSR COO_to_CSR(SparseMatrix_COO* sparse_matrix) {
    // Sort COO input matrix
    SparseMatrix_COO_sort(sparse_matrix);

    // Initialize CSR output matrix
    SparseMatrix_CSR output_matrix = SparseMatrix_CSR_init(sparse_matrix->rows, sparse_matrix->cols);

    // To see if we're in a new row
    int previous_row_index = -1;
    int new_row = 0;

    // Go through all COO non-zero entries
    for (int i = 0; i < sparse_matrix->number_of_nonzeroes; i++) {
        // Get necessary value for entry
        int row = sparse_matrix->nonzero_row_indices[i];
        int col = sparse_matrix->nonzero_col_indices[i];
        double value = sparse_matrix->values[i];

        // Check if it's a new row
        if (sparse_matrix->nonzero_row_indices[i] != previous_row_index) {
            new_row = 1;
        }
        
        // Add it to CSR matrix
        SparseMatrix_CSR_add_entry(&output_matrix, row, col, value, new_row);

        // Reset new row value and previous row value
        if (new_row) {
            new_row = 0;
        }
        previous_row_index = row;
    }

    return output_matrix;
}

void print_sparsematrix_COO(const SparseMatrix_COO* sparse_matrix) {
    for (int i = 0; i < sparse_matrix->rows; i++) {
        for (int j = 0; j < sparse_matrix->cols; j++) {
            printf("%lf ", SparseMatrix_COO_get_entry(sparse_matrix, i, j));
        }

        printf("\n");
    }
}

// Sort entries of arrays in COO matrix in order of row -> col
void SparseMatrix_COO_sort(SparseMatrix_COO* sparse_matrix) {
    // Check usage
    if (sparse_matrix->number_of_nonzeroes < 2) {
        return;
    }

    // Intermediate sorted arrays after first pass (column pass)
    int intermdiate_col[sparse_matrix->number_of_nonzeroes];
    int intermediate_row[sparse_matrix->number_of_nonzeroes];
    double intermediate_values[sparse_matrix->number_of_nonzeroes];

    // Final sorted arrays after second pass (row pass)
    int sorted_col[sparse_matrix->number_of_nonzeroes];
    int sorted_row[sparse_matrix->number_of_nonzeroes];
    double sorted_values[sparse_matrix->number_of_nonzeroes];

    // Counts and next position arrays
    int col_counts[sparse_matrix->cols];
    int row_counts[sparse_matrix->rows];
    int next_pos_col[sparse_matrix->cols];
    int next_pos_row[sparse_matrix->rows];

    // Initialize
    memset(col_counts, 0, sizeof(col_counts));
    memset(row_counts, 0, sizeof(row_counts));
    memset(next_pos_col, 0, sizeof(next_pos_col));
    memset(next_pos_row, 0, sizeof(next_pos_row));

    for (int i = 0; i < sparse_matrix->number_of_nonzeroes; i++) {
        col_counts[sparse_matrix->nonzero_col_indices[i]] += 1;
        row_counts[sparse_matrix->nonzero_row_indices[i]] += 1;
    }

    for (int i = 1; i < sparse_matrix->cols; i++) {
        next_pos_col[i] = next_pos_col[i - 1] + col_counts[i - 1];
    }

    for (int i = 1; i < sparse_matrix->rows; i++) {
        next_pos_row[i] = next_pos_row[i - 1] + row_counts[i - 1];
    }

    // First pass
    for (int i = 0; i < sparse_matrix->number_of_nonzeroes; i++) {
        int col_index = sparse_matrix->nonzero_col_indices[i];

        int new_index = next_pos_col[col_index]++;

        intermdiate_col[new_index] = col_index;
        intermediate_row[new_index] = sparse_matrix->nonzero_row_indices[i];
        intermediate_values[new_index] = sparse_matrix->values[i];
    }

    // Second pass
    for (int i = 0; i < sparse_matrix->number_of_nonzeroes; i++) {
        int row_index = intermediate_row[i];

        int new_index = next_pos_row[row_index]++;

        sorted_col[new_index] = intermdiate_col[i];
        sorted_row[new_index] = intermediate_row[i];
        sorted_values[new_index] = intermediate_values[i];
    }

    // Push sorted values to COO matrix
    for (int i = 0; i < sparse_matrix->number_of_nonzeroes; i++) {
        sparse_matrix->nonzero_col_indices[i] = sorted_col[i];
        sparse_matrix->nonzero_row_indices[i] = sorted_row[i];
        sparse_matrix->values[i] = sorted_values[i];
    }
}