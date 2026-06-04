import json
import re
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
WORKSPACE = SCRIPT_DIR.parent
NETLIST_PATH = WORKSPACE / "netlist.sp"

# On MacOS
if (sys.platform == "darwin"):
    CLSPICE_EXE = WORKSPACE / "clspice"
else:
    CLSPICE_EXE = WORKSPACE / "clspice.exe"

REFERENCE_DIR = SCRIPT_DIR / "reference_results"
DEFAULT_SUITE_DIR = SCRIPT_DIR / "golden"
TOLERANCE = 1e-6
GREEN = "\033[32m"
RED = "\033[31m"
RESET = "\033[0m"


def compare_maps(expected, actual):
    keys = sorted(set(expected) | set(actual), key=lambda key: (key[0], int(key[1:]) if key[1:].isdigit() else key))
    rows = []
    max_err = 0.0
    passed = True

    for key in keys:
        exp = expected.get(key)
        act = actual.get(key)
        if exp is None or act is None:
            rows.append((key, exp, act, None, False))
            passed = False
            continue

        err = abs(exp - act)
        ok = err <= TOLERANCE
        rows.append((key, exp, act, err, ok))
        max_err = max(max_err, err)
        passed = passed and ok

    return passed, max_err, rows


def resolve_suite_dir(arg):
    if arg is None:
        return DEFAULT_SUITE_DIR

    suite_dir = Path(arg)
    if suite_dir.is_absolute():
        return suite_dir

    candidates = [SCRIPT_DIR / suite_dir, WORKSPACE / suite_dir]
    for candidate in candidates:
        if candidate.exists():
            return candidate

    return candidates[0]


def load_reference_suite(suite_dir):
    reference_path = REFERENCE_DIR / f"{suite_dir.name}.json"
    if not reference_path.exists():
        raise FileNotFoundError(f"missing reference file: {reference_path}")

    return json.loads(reference_path.read_text(encoding="ascii"))


def parse_clspice_output(stdout):
    node_pattern = re.compile(r"^v(\d+)\s*=\s*([-+]?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?)\s+V$", re.MULTILINE)
    source_pattern = re.compile(r"^I\((V\w+)\)\s*=\s*([-+]?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?)\s+A$", re.MULTILINE)

    node_voltages = {f"v{match.group(1)}": float(match.group(2)) for match in node_pattern.finditer(stdout)}
    source_currents = {match.group(1): float(match.group(2)) for match in source_pattern.finditer(stdout)}
    return node_voltages, source_currents


def run_clspice_once(netlist_text):
    NETLIST_PATH.write_text(netlist_text, encoding="ascii")
    completed = subprocess.run(
        [str(CLSPICE_EXE)],
        input="\n",
        capture_output=True,
        text=True,
        cwd=WORKSPACE,
        check=False,
    )

    if completed.returncode != 0:
        raise RuntimeError(
            f"clspice exited with code {completed.returncode}\nstdout:\n{completed.stdout}\nstderr:\n{completed.stderr}"
        )

    return parse_clspice_output(completed.stdout), completed.stdout


def run_case(netlist_path, reference_case):
    netlist_text = netlist_path.read_text(encoding="ascii")
    (actual_nodes, actual_sources), raw_output = run_clspice_once(netlist_text)

    expected_nodes = reference_case.get("nodes", {})
    expected_sources = reference_case.get("voltage_sources", {})

    v_ok, v_max_err, v_rows = compare_maps(expected_nodes, actual_nodes)
    i_ok, i_max_err, i_rows = compare_maps(expected_sources, actual_sources)
    passed = v_ok and i_ok

    print("=" * 80)
    print(f"TEST: {netlist_path.stem}")
    print(f"RESULT: {GREEN}PASS{RESET}" if passed else f"RESULT: {RED}FAIL{RESET}")
    print(f"Max |dv|: {v_max_err:.3e} V")
    print(f"Max |di|: {i_max_err:.3e} A")

    print("\nNode voltages:")
    for key, exp, act, err, ok in v_rows:
        if exp is None or act is None:
            print(f"  {key:>5}: expected={exp} actual={act} -> MISSING")
        else:
            status = "OK" if ok else "BAD"
            print(f"  {key:>5}: expected={exp:+.9f} actual={act:+.9f} |err|={err:.3e} {status}")

    print("\nVoltage source currents:")
    for key, exp, act, err, ok in i_rows:
        if exp is None or act is None:
            print(f"  {key:>5}: expected={exp} actual={act} -> MISSING")
        else:
            status = "OK" if ok else "BAD"
            print(f"  {key:>5}: expected={exp:+.9f} actual={act:+.9f} |err|={err:.3e} {status}")

    if not passed:
        print("\nRaw clspice output for debugging:")
        print(raw_output)

    print()
    return passed


def main():
    if not CLSPICE_EXE.exists():
        raise FileNotFoundError(f"missing executable: {CLSPICE_EXE}")

    suite_dir = resolve_suite_dir(sys.argv[1] if len(sys.argv) > 1 else None)
    if not suite_dir.exists():
        raise FileNotFoundError(f"missing suite directory: {suite_dir}")

    netlists = sorted(suite_dir.glob("*.sp"))
    if not netlists:
        raise FileNotFoundError(f"no .sp files found in {suite_dir}")

    reference_suite = load_reference_suite(suite_dir)

    overall = True
    for netlist_path in netlists:
        if netlist_path.stem not in reference_suite:
            raise KeyError(f"missing reference case for {netlist_path.stem}")
        overall = run_case(netlist_path, reference_suite[netlist_path.stem]) and overall

    print("=" * 80)
    print(f"OVERALL: {GREEN}PASS{RESET}" if overall else f"OVERALL: {RED}FAIL{RESET}")


if __name__ == "__main__":
    main()
