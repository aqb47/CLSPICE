#include "sparsematrix.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

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

int SparseMatrix_COO_add_entry(SparseMatrix_COO* sparse_matrix, int entry_row, int entry_col, double value) {
    // Can't add zero valued entry
    if (value == 0) {
        return 1;
    }

    // Bounds check
    if (entry_row + 1 > sparse_matrix->rows || entry_col + 1 > sparse_matrix->cols) {
        return 2;
    }

    // Reallocate values, rows, cols values
    double* temp_values = realloc(sparse_matrix->values, sizeof(double) * (sparse_matrix->number_of_nonzeroes + 1));
    int* temp_row = realloc(sparse_matrix->nonzero_row_indices, sizeof(int) * (sparse_matrix->number_of_nonzeroes + 1));
    int* temp_col = realloc(sparse_matrix->nonzero_col_indices, sizeof(int) * (sparse_matrix->number_of_nonzeroes + 1));

    // If reallocation goes wrong
    if (temp_values == NULL || temp_row == NULL || temp_col == NULL) {
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

void SparseMatrix_COO_free(SparseMatrix_COO* sparse_matrix) {
    // Free dynamically allocated values if they aren't null already
    if (sparse_matrix->values == NULL && sparse_matrix->nonzero_row_indices == NULL && sparse_matrix->nonzero_col_indices == NULL) {
        return;
    }
    free(sparse_matrix->values);
    free(sparse_matrix->nonzero_row_indices);
    free(sparse_matrix->nonzero_col_indices);

    // Reset data
    sparse_matrix->values = NULL;
    sparse_matrix->nonzero_col_indices = NULL;
    sparse_matrix->nonzero_row_indices = NULL;
}

double SparseMatrix_COO_get_entry(SparseMatrix_COO* sparse_matrix, int entry_row, int entry_col) {
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

void print_sparsematrix_COO(SparseMatrix_COO* sparse_matrix) {
    for (int i = 0; i < sparse_matrix->rows; i++) {
        for (int j = 0; j < sparse_matrix->cols; j++) {
            printf("%lf ", SparseMatrix_COO_get_entry(sparse_matrix, i, j));
        }

        printf("\n");
    }
}