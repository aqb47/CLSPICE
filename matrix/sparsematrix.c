#include "sparsematrix.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

SparseMatrix_COO SparseMatrix_COO_init(int rows, int cols) {
    SparseMatrix_COO sparse_matrix = {
        .rows = rows,
        .cols = cols,
        .number_of_nonzeroes = 0
    };

    return sparse_matrix;
}

int SparseMatrix_COO_add_entry(SparseMatrix_COO* sparse_matrix, int entry_row, int entry_col, double value) {
    if (value == 0) {
        return 1;
    }

    if (entry_row + 1 > sparse_matrix->rows || entry_col + 1 > sparse_matrix->cols) {
        return 2;
    }
    
    double* temp_values = realloc(sparse_matrix->values, sizeof(sparse_matrix->values) + sizeof(value));
    int* temp_row = realloc(sparse_matrix->nonzero_row_indices, sizeof(sparse_matrix->nonzero_row_indices) + sizeof(entry_row));
    int* temp_col = realloc(sparse_matrix->nonzero_col_indices, sizeof(sparse_matrix->nonzero_col_indices) + sizeof(entry_col));


    if (temp_values == NULL || temp_row == NULL || temp_col == NULL) {
        return 3;
    }

    sparse_matrix->values = temp_values;
    sparse_matrix->nonzero_row_indices = temp_row;
    sparse_matrix->nonzero_col_indices = temp_col;

    int index_to_be_added = sparse_matrix->number_of_nonzeroes;

    sparse_matrix->values[index_to_be_added] = value;
    sparse_matrix->nonzero_row_indices[index_to_be_added] = entry_row;
    sparse_matrix->nonzero_col_indices[index_to_be_added] = entry_col;
    
    sparse_matrix->number_of_nonzeroes += 1;

    return 0;
}

void SparseMatrix_COO_free(SparseMatrix_COO* sparse_matrix) {
    free(sparse_matrix->values);
    free(sparse_matrix->nonzero_row_indices);
    free(sparse_matrix->nonzero_col_indices);
}

double SparseMatrix_COO_get_entry(SparseMatrix_COO* sparse_matrix, int entry_row, int entry_col) {
    if (entry_row + 1 > sparse_matrix->rows || entry_col + 1 > sparse_matrix->cols) {
        return NAN;
    }

    for (int i = 0; i < sparse_matrix->number_of_nonzeroes; i++) {
        if (sparse_matrix->nonzero_row_indices[i] == entry_row && sparse_matrix->nonzero_col_indices[i] == entry_col) {
            return sparse_matrix->values[i];
        }
    }

    return 0;
}

void print_sparsematrix_COO(SparseMatrix_COO* sparse_matrix) {
    for (int i = 0; i < sparse_matrix->rows; i++) {
        for (int j = 0; j < sparse_matrix->cols; j++) {
            printf("%lf ", SparseMatrix_COO_get_entry(sparse_matrix, i, j));
        }

        printf("\n");
    }
}