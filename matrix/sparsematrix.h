#ifndef SPARSEMATRIX_H
#define SPARSEMATRIX_H

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

// Coordinate format sparse matrix - for building a sparse matrix
typedef struct {
    // Total rows and columns
    int rows;
    int cols;

    // Total non-zero entries
    int number_of_nonzeroes;

    // Arrays of row, col and values of each non-zero entry where the index is position of that non-zero
    int* nonzero_row_indices;
    int* nonzero_col_indices;
    double* values;
} SparseMatrix_COO;

// Compressed sparse row matrix - for operating with a sparse matrix
typedef struct {
    // Total rows and columns
    int rows;
    int cols;

    // Total non-zero entries
    int number_of_nonzeroes;

    // Position of first non-zero value of each row
    int* row_ptr;
    // Corresponding column value array for values
    int* nonzero_col_indices;
    // Non-zero value array
    double* values;
} SparseMatrix_CSR;

// Initialize a coordinate form sparse matix
SparseMatrix_COO SparseMatrix_COO_init(int rows, int cols);
// Free allocated data from coordinate form sparse matrix
void SparseMatrix_COO_free(SparseMatrix_COO* sparse_matrix);

// Initialize a compressed row sparse matrix
SparseMatrix_CSR SparseMatrix_CSR_init(int rows, int cols);
// Free allocated data from compressed row sparse matrix
void SparseMatrix_CSR_free(SparseMatrix_CSR* sparse_matrix);

// Add an entry at zero-based row, col position of coordinate form sparse matrix
int SparseMatrix_COO_add_entry(SparseMatrix_COO* sparse_matrix, int entry_row, int entry_col, double value);
// Get entry value at zero-based row and column indices from coordinate form sparse matrix, return NAN if something goes wrong
double SparseMatrix_COO_get_entry(const SparseMatrix_COO* sparse_matrix, int entry_row, int entry_col);

// Convert COO matrix to CSR matrix. As the CSR matrix is dynamically allocated, it must be freed too
SparseMatrix_CSR COO_to_CSR(SparseMatrix_COO* sparse_matrix);

// Print all entries of a coordinate form sparse matrix
void print_sparsematrix_COO(const SparseMatrix_COO* sparse_matrix);
// Print all entries of a compressed row sparse matrix
void print_sparsematrix_CSR(const SparseMatrix_CSR* sparse_matrix);

#endif