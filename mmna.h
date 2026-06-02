// MNA with sparse matrices

#ifndef MMNA_H
#define MMNA_H

#include "circuit.h"
#include "sparsematrix.h"

int build_input_output_sparsematrix(SparseMatrix_COO* input, DenseVector* output, Circuit circuit);

#endif