#ifndef SPARSEMATRIX_H
#define SPARSEMATRIX_H

#include <stdlib.h>
#include <math.h>
#include <stdio.h>

// Coordinate format sparse matrix 
typedef struct {
    int rows;
    int cols;

    int number_of_nonzeroes;

    // Arrays of row, col and values of each non-zero entry where the index is position of that non-zero
    int* nonzero_row_indices;
    int* nonzero_col_indices;
    double* values;
} SparseMatrix_COO;

SparseMatrix_COO SparseMatrix_COO_init(int rows, int cols);
void SparseMatrix_COO_free(SparseMatrix_COO* sparse_matrix);

int SparseMatrix_COO_add_entry(SparseMatrix_COO* sparse_matrix, int entry_row, int entry_col, double value);
double SparseMatrix_COO_get_entry(SparseMatrix_COO* sparse_matrix, int entry_row, int entry_col);

void print_sparsematrix_COO(SparseMatrix_COO* sparse_matrix);

#endif