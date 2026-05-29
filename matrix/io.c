#include <stdio.h>
#include <math.h>
#include "matrix.h"

static int count_digits(double value);
static int get_max_width(Matrix A);
static void print_matrix_ends(int width);

// Print the matrix A with border
void print_matrix(Matrix A) {
    int rows = A.rows;
    int cols = A.cols;

    // In case of an empty matrix
    if ((rows == EMPTY_MATRIX.rows) && (cols == EMPTY_MATRIX.cols) && (A.data[0][0] == 0)) {
        printf("ERROR");
        return;
    }

    // How many digits after decimal point to be shown
    int precision = 4;

    // How many spaces there are between elements
    int padding = 4;

    // Calculate total width of each element for formatting
    int total_width = get_max_width(A) + precision;

    int ends_width = total_width * cols + padding * cols + 2; 

    // Top border
    print_matrix_ends(ends_width);
    printf("\n");

    // For each row
    for (int i = 0; i < rows; i++) {
        // For each column in row

        // Left border
        printf("| ");

        for (int j = 0; j < cols; j++) {
            // Prevent -0.0000 - this specifically relates to precision
            if (fabs(A.data[i][j]) < 0.00005) A.data[i][j] = 0.0;

            // Print the element 
            printf("  %*.*lf  ", total_width, precision, A.data[i][j]);
        }

        // Right border
        printf(" |\n");
    }

    // Bottom border
    print_matrix_ends(ends_width);
    printf("\n");
}

// Print the top and bottom borders of the matrix
void print_matrix_ends(int width) {
    printf("-");
    
    for (int i = 0; i < width; i++) {
        printf(" ");
    }
    
    printf("-");
}

// Get max width of the integer parts of elements in the matrix for formatting
int get_max_width(Matrix A) {
    int rows = A.rows;
    int cols = A.cols;

    int max_width = 0;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double value = A.data[i][j];
            int width;

            // +1 for decimal point
            width = count_digits(value) + 1; 

            if (width > max_width) {
                max_width = width;
            }
        }
    }

    return max_width;
}

// Get width of integer part of a double
int count_digits(double value) {
    int digits = 0;

    // Specifically for 0.00..
    if (fabs(value) < 1e-9) {
        digits++;
        return digits;
    }

    // Account for negative sign
    if (value < 0) {
        digits++; 
        value = -value; 
    }

    // Account for leading zero before decimal point
    if (value < 1) {
        digits++; 
    }

    digits += floor(log10(value)) + 1;

    return digits;
}

// Print rows, cols and entries of a matrix to a file without formatting
int save_to_file(Matrix A, const char* filename) {
    FILE *destFile;
    int rows = A.rows;
    int cols = A.cols;

    // Open file
    destFile = fopen(filename, "a");
    if (destFile == NULL) {
        return 1;
    }

    // Print rows and cols of matrix
    fprintf(destFile, "%i %i\n", rows, cols);

    // Print every entry of matrix
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fprintf(destFile, "%.4lf ", A.data[i][j]);
        }

        fprintf(destFile, "\n");
    }

    // Close file
    fclose(destFile);
    return 0;
}

// Load a matrix from a file in form -
// rows cols
// entries
// Offset is how many bytes we've read before. This is necessary for importing multiple matrices from a single file
Matrix load_from_file(const char* filename, long* offset_ptr) {
    FILE *srcFile;
    
    int rows, cols;

    // Open file and check for null pointers
    srcFile = fopen(filename, "r");
    
    if (srcFile == NULL || offset_ptr == NULL) {
        return EMPTY_MATRIX;
    }

    // Apply offset
    fseek(srcFile, *offset_ptr, SEEK_SET);

    // Record where we begin reading file
    long byte_start = ftell(srcFile);

    // Read rows and cols number
    fscanf(srcFile, "%d %d\n", &rows, &cols);

    if (rows > MAX_SIZE || cols > MAX_SIZE) {
        *offset_ptr += ftell(srcFile) - byte_start;
        return EMPTY_MATRIX;
    }

    // Initialize matrix
    Matrix A = {{{0}}, rows, cols};

    // Read entries
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double entry_value;
            
            fscanf(srcFile, "%lf", &entry_value);
            A.data[i][j] = entry_value;
        }
    }

    // Where we end reading file
    long byte_end = ftell(srcFile);

    // Change offset
    *offset_ptr += byte_end - byte_start;

    // Close file and return matrix
    fclose(srcFile);

    return A;
}