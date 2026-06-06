* Future case: VCVS gain test
* Ename N+ N- NC+ NC- gain
V1 1 0 8
V2 3 0 4
R1 1 2 1.2k
R2 2 0 2k
E1 2 0 1 0 3
R3 2 3 2.2k
R4 3 0 1.8k
*E2 3 0 2 0 0.4999
.end

* Shunt resistors to avoid singular matrix during testing
Rsh1 1 0 1G
Rsh2 2 0 1G
Rsh3 3 0 1G
