# Test Netlists

This folder contains reusable SPICE netlists for regression testing.

- `golden/` contains DC circuits that should pass with the current solver.
- `dependent_sources/` contains future test vectors for controlled sources once those devices are implemented.

Usage:

```powershell
python test_netlists/compare_cspice.py
```

To run the dependent-source suite LATER:

```powershell
python test_netlists/compare_cspice.py dependent_sources
```

The comparator resolves paths relative to this directory, so it is self-contained within `test_netlists`.
