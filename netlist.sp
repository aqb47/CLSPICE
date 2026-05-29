* V1 sets node 1 to 12V
* R1 connects node 1 to node 2
* I1 injects 1mA into node 2
* R2 and R3 both pull node 2 to ground
V1 1 0 12
R1 1 2 2000
R2 2 0 4000
R3 2 0 4000
I1 0 2 0.001
.end