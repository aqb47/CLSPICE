# Test Netlists

This folder contains reusable SPICE netlists for regression testing.

- `golden/` contains DC circuits that should pass with the current solver.
- `dependent_sources/` contains future test vectors for controlled sources once those devices are implemented.

Usage:

```powershell
python compare_cspice_tests.py test_netlists/golden
```

After dependent sources are implemented, the same command can be pointed at `test_netlists/dependent_sources`.
