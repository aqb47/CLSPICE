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

// Initialize a coordinate form sparse matix
SparseMatrix_COO SparseMatrix_COO_init(int rows, int cols);
// Add an entry at zero-based row, col position of coordinate form sparse matrix
void SparseMatrix_COO_free(SparseMatrix_COO* sparse_matrix);

// Free allocated data from coordinate form sparse matrix
int SparseMatrix_COO_add_entry(SparseMatrix_COO* sparse_matrix, int entry_row, int entry_col, double value);
// Get entry value at zero-based row and column indices from coordinate form sparse matrix, return NAN if something goes wrong
double SparseMatrix_COO_get_entry(SparseMatrix_COO* sparse_matrix, int entry_row, int entry_col);

// Print all entries of a coordinate form sparse matrix
void print_sparsematrix_COO(SparseMatrix_COO* sparse_matrix);

#endif