# Test Netlists

This folder contains reusable SPICE netlists for regression testing.

- `golden/` contains DC circuits that should pass with the current solver.
- `dependent_sources/` contains test vectors for controlled sources.

Usage:

```powershell
python test_netlists/compare_clspice.py
```

To run the dependent-source suite LATER:

```powershell
python test_netlists/compare_clspice.py dependent_sources
```

The comparator resolves paths relative to this directory, so it is self-contained within `test_netlists`.
