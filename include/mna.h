// MNA = Modified Nodal Analysis. This header file will be for building the input and output matrices
// Basic equation for MNA = [G B] [v] = [I]
//                          [C D] [i]   [E]
//                            |     |     |
//                         input unknown output
// G = Conductance matrix, B = Incidence matrix, C = Transposed Incidence matrix, D = Control matrix
// v = Node voltages, i = Voltage source currents
// I = Current source currents, E = Voltage source voltages

#ifndef MNA_H
#define MNA_H

#include "circuit.h"
#include "matrix.h"

int build_input_output_matrix(Matrix* input, Matrix* output, Circuit circuit);

#endif