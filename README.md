# CSPICE
A simple SPICE (Simulation Program with Integrated Circuit Emphasis) implementation in C

## What is SPICE
** Please note that my following explanation glosses over many details about MNA and is applicable only to resistor and independent voltage/ current source circuits. I wrote this for people like me with a very basic understanding of Nodal Analysis wanting to know more about the process SPICE programs use.**

Essentially, all SPICE programs analyse a 'netlist' to get information about nodes, circuit elements and element values within a circuit, and perform an operation called MNA (Modified Nodal Analysis) based on that information. 

Classical nodal analysis is the systematic application of KCL on nodes within electric circuits. You consider one node a 'ground' with 0V and work with node voltages with respect to the ground. For 'n' number of nodes, you solve n - 1 simultaneous linear equations (without voltage sources).

But since nodal analysis uses KCL, voltage sources with an unknown current flow are difficult to work with. Normally to account for them the common non-reference nodes connected to that voltage source are considered a single 'supernode' (which I think sounds super goofy) and you use KVL instead. 

MNA is nodal analysis but you simply assign a current flow to the voltage sources as another variable anyways. Thus by this process you have to solve n - 1 + m simultaneous equations, where 'n' is the node number and 'm' is the voltage source number.

MNA can be further generalised by an interesting matrix equation.

[G B] * [v] = [I]
[C D]   [i]   [E]

Where,
G = Conductance matrix
B = Incidence matrix
C = Transposed incidence matrix
D = Zero matrix (in our case)

v = Node voltage vector
i = Voltage source currents vector

I = Node current vector 
E = Voltage source voltage vector

### Conductance Matrix (G)
This is a square matrix of dimensions n x n for n amount of non-reference nodes. 

The entry G(i, j) or ith row and jth column entry (i =/= j) represents the negative sum of conductances of common resistors between ith and 'jth non-reference nodes.

The entry G(k, k) or diagonal entries represent positive sum of conductances of resistors attached to kth non-reference nodes.

### Incidence Matrix (B)
This is a matrix of dimension n x m, for n amount of non-reference nodes and make amount of independent voltage sources.

The entry B(i, j) tells us about the polarity of jth voltage source in relation to ith non-reference node. 

If B(i, j) = +1, node i is connected to positive terminal of jth voltage source.

If B(i, j) = -1, node i is connected to negative terminal of jth voltage source.

And if B(i, j) = 0, node i and jth voltage source are not connected to each other.

The matrix C in the input matrix is also just the transpose of this matrix B.

### Node Current Vector (I)
This is a matrix of dimension n x 1, for n non-reference nodes.

An entry I(i, 1) is the total amount of current entering a node (+ for entering and - for exiting).

### Voltage source voltage vector (E)
This is a matrix of dimension m x 1, for m independent voltage sources.

An entry E(i, 1) is the magnitude of the voltage across the ith voltage source.

### Node voltage vector (v) and voltage source current vector (i)
These are our unknowns. v is of dimension n x 1 and i is of dimension m x 1, with n being non-reference nodes and m being voltage sources.