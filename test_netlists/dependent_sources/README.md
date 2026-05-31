# Dependent Source Regression Suite

These are future test vectors for when the solver supports controlled sources.

Planned coverage:
- `G` VCCS
- `F` CCCS
- `E` VCVS
- `H` CCVS

Once supported, run them the same way as the golden suite:

```powershell
python test_netlists/compare_cspice.py dependent_sources
```

These files use standard SPICE controlled-source syntax and are not expected to run correctly until the parser and MNA stamping are extended.
