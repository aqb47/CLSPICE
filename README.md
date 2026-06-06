# CLSPICE
A simple SPICE (Simulation Program with Integrated Circuit Emphasis) implementation in C

## How to Use
Clone the repository and compile the program with MakeFile.

e.g. If you use mingw32 you can run

```mingw32-make -f MakeFile```

Then, input a netlist within 'netlist.sp'.
 
e.g.
```
* you can make a comment like this
* normal element format: NAME NODE+ NODE- VALUE
* controlled voltage source element format: NAME NODE+ NODE- CONTROL_NODE+ CONTROL_NODE- GAIN 
* controlled current source element format: NAME NODE+ NODE- CONTROL_VOLTAGE_SOURCE_NAME GAIN
* suffixes are also supported - u (micro), m (milli), k (kilo), M (mega), g (giga), t (tera)
V1 1 0 24
R1 1 2 1500
R2 2 0 1k
I1 2 3 0.003
R3 3 0 2200
R4 1 3 3300
V2 2 4 6
R5 4 0 4700
I2 0 4 15m
.end
```

Make sure your node names are numeric and sequential from 0 (which is always the ground node).

The circuit element name must start with a valid element type and is limited to 7 characters in length.

Supported element types:
 
• V (DC independent voltage source)

• I (DC independent current source)

• R (fixed resistor)

• E (DC voltage controlled voltage source)

• F (DC current controlled current source)

• G (DC voltage controlled current source)

• H (DC current controlled voltage source)

Then run `clspice.exe` and the output (node voltages + branch currents + voltage source currents) should be outputted to the terminal. 

**Note:** Voltage source currents should always be negative in the output. This is because MNA assumes passive sign convention for all sources. That means current flows from positive to negative. But voltage sources actually have current flowing from negative to positive, because they're active elements. Hence the negative sign.  

Also if you have a current-controlled dependent source that depends on current flowing through an element that isn't a voltage source (like a resistor), you create another node ahead of that element and put a voltage source of 0V in front of it, and put that as the control voltage source. 

e.g 
```(1)----R1-----(2)```

Lets say we want the current through R1 flowing from node-1 to node-2 as the controlling current for a dependent source

```(1)----R1---(3)---Vzero---(2) ```

Now we can just set Vzero = 0V, and literally put 'Vzero' as the control name for F/ G element types (CCCS and CCVS respectively). Current-controlled sources will always take a voltage source as the control element, because this allows us to manipulate the MNA equations (which have voltage source currents as an output) for stamping those dependent sources.

## What I Want to Add
• **Gaussian elimination instead of the inverse matrix method currently being used for solving equations, as matrix inversion by cofactor expansion is very inefficient at O(n!) complexity.**

Status: Done and implemented.

• **Dynamic and sparse matrices, this will make storing matrices more memory-efficient for really large circuits, and LU decomposition for solving those sparse matrices.**

Status: Done-ish? I got as far as implementing the input-output matrix building for sparse matrices. But when I tried to solve them I realized sparse matrices are much more difficult to solve than dense matrices, so I'm sticking to dynamic dense matrices with the hope I'll come back to sparse matrices later :P

• **Dependent voltage/ current sources (VCVS, CCVS, VCCS, CCCS)**

Status: Done and implemented, but needs more testing.

• **Capacitors and inductors**

Status: Not started, this'll involve implementing AC transient analysis in the circuit simulation.

• **A GUI? (Might be too ambitious)**

Status: Yeaah I don't even know where to start with this one.

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

I'd add details about the dependent sources stamping but this README is already really long. So that's pretty much it, I hoped you learnt something from this little learning exercise of a repo.
