# CSPICE
A simple SPICE (Simulation Program with Integrated Circuit Emphasis) implementation in C

## How to Use
Clone the repository and compile the program with MakeFile.

e.g. If you use mingw32 you can run

```mingw32-make -f MakeFile```

Then, input a netlist within 'netlist.sp'.
 
e.g.
```
* you can make a comment like this
* element format: NAME NODE+ NODE- VALUE
V1 1 0 24
R1 1 2 1500
R2 2 0 1000
I1 2 3 0.003
R3 3 0 2200
R4 1 3 3300
V2 2 4 6
R5 4 0 4700
I2 0 4 0.0015
.end
```

Make sure your node names are numeric and sequential from 0 (which is always the ground node).

The circuit element name must start with a valid element type and is limited to 3 characters in length.

Supported element types: V (DC independent voltage source), I (DC independent current source), R (fixed resistor).

Then run `cspice.exe` and the output (node voltages + voltage source currents) should be outputted to the terminal. 

## What I Want to Add
• Gaussian elimination instead of the inverse matrix method currently being used for solving equations, as matrix inversion by cofactor expansion is very inefficient at O(n!) complexity.

• Dependent voltage/ current sources (VCVS, CCVS, VCCS, CCCS)

• Capacitors and inductors


## What is SPICE
**Please note that my following explanation glosses over many details about MNA and is applicable only to circuits consisting of resistors and independent voltage/ current sources (capacitors and inductors complicate things). I wrote this for people like me with a very basic understanding of Nodal Analysis, wanting to know more about the process that SPICE programs use.**

Essentially, all SPICE programs analyse a 'netlist' to get information about nodes, circuit elements and element values within a circuit, and perform an operation called MNA (Modified Nodal Analysis) based on that information. 

Classical nodal analysis is the systematic application of KCL on nodes within electric circuits. You consider one node a 'ground' with 0V and work with node voltages with respect to the ground. For `n` number of nodes, you solve `n - 1` simultaneous linear equations (without voltage sources).

But since nodal analysis uses KCL, voltage sources with an unknown current flow are difficult to work with. Normally to account for them the common non-reference nodes connected to that voltage source are considered a single 'supernode' (which I think sounds super goofy) and you use KVL instead. 

MNA is nodal analysis but you simply assign a current flow to the voltage sources as another variable anyways. Thus by this process you have to solve `n - 1 + m` simultaneous equations, where `n` is the node number and `m` is the voltage source number.

MNA can be further generalised by an interesting matrix equation.

```
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
```

### Conductance Matrix (G)
This is a square matrix of dimensions `n x n` for `n` amount of non-reference nodes. 

The entry `G(i, j)` or `i`th row and `j`th column entry `(i =/= j)` represents the negative sum of conductances of common resistors between `i`th and `j`th non-reference nodes.

The entry `G(k, k)` or diagonal entries represent positive sum of conductances of resistors attached to `k`th non-reference nodes.

### Incidence Matrix (B)
This is a matrix of dimension `n x m`, for `n` amount of non-reference nodes and `m` amount of independent voltage sources.

The entry `B(i, j)` tells us about the polarity of `j`th voltage source in relation to `i`th non-reference node. 

If `B(i, j) = +1`, node `i` is connected to positive terminal of `j`th voltage source.

If `B(i, j) = -1`, node `i` is connected to negative terminal of `j`th voltage source.

And if `B(i, j) = 0`, node `i` and `j`th voltage source are not connected to each other.

The matrix C in the input matrix is also just the transpose of this matrix B.

### Node Current Vector (I)
This is a matrix of dimension `n x 1`, for `n` non-reference nodes.

An entry `I(i, 1)` is the total amount of current entering `i`th node (+ for entering and - for exiting).

### Voltage source voltage vector (E)
This is a matrix of dimension `m x 1`, for `m` independent voltage sources.

An entry `E(i, 1)` is the magnitude of the voltage across the `i`th voltage source.

### Node voltage vector (v) and voltage source current vector (i)
These are our unknowns. v is of dimension `n x 1` and i is of dimension `m x 1`, with `n` being non-reference nodes and `m` being voltage sources.