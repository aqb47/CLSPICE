# Test Netlists

This folder contains reusable SPICE netlists for regression testing.

- `golden/` contains DC circuits that should pass with the current solver.
- `dependent_sources/` contains future test vectors for controlled sources once those devices are implemented.

Usage:

```powershell
python test_netlists/run_regression.py
```

To run the dependent-source suite later:

```powershell
python test_netlists/run_regression.py dependent_sources
```

The runner resolves paths relative to this directory, so it is self-contained within `test_netlists`.
