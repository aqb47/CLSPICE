#ifndef MATRIX_H
#define MATRIX_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Empty matrix to return when an operation fails
#define EMPTY_MATRIX (Matrix) {NULL, 0, 0} 

// Matrix struct
typedef struct {
    double** data;
    
    int rows;
    int cols;
} Matrix;

Matrix Matrix_init(int rows, int cols);

Matrix gaussian_elimination(Matrix coefficient_matrix, Matrix result_matrix);

Matrix transpose(Matrix input_matrix);

void Matrix_free(Matrix* matrix);

void print_matrix(Matrix A);

#endif