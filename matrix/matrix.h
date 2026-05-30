#ifndef MATRIX_H
#define MATRIX_H

#include <stdio.h>
#include <math.h>

// 5x5 matrix as max size
#define MAX_SIZE 11
// Empty matrix to return when an operation fails
#define EMPTY_MATRIX (Matrix) {{{0}}, 0, 0} 

// A matrix will always have max_size x max_size dimensions, just specified with rows and cols
typedef struct {
    double data[MAX_SIZE][MAX_SIZE];
    int rows;
    int cols;
} Matrix;

Matrix add(Matrix A, Matrix B);
Matrix subtract(Matrix A, Matrix B);

Matrix multiply(Matrix A, Matrix B);

Matrix transpose(Matrix A);

double determinant(Matrix A);

Matrix inverse(Matrix A);

Matrix gaussian_elimination(Matrix coefficient_matrix, Matrix result_matrix);

void print_matrix(Matrix A);

int save_to_file(Matrix A, const char* filename);

Matrix load_from_file(const char* filename, long* offset_ptr);

#endif 