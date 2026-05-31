import re
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
WORKSPACE = SCRIPT_DIR.parent
NETLIST_PATH = WORKSPACE / "netlist.sp"
CSPICE_EXE = WORKSPACE / "cspice.exe"
TOLERANCE = 1e-6
DEFAULT_SUITE_DIR = SCRIPT_DIR / "golden"
GREEN = "\033[32m"
RED = "\033[31m"
RESET = "\033[0m"


def parse_value(raw: str) -> float:
    raw = raw.strip()
    suffix_map = {
        "u": 1e-6,
        "m": 1e-3,
        "k": 1e3,
        "M": 1e6,
        "g": 1e9,
        "t": 1e12,
    }

    if raw and raw[-1].isalpha():
        suffix = raw[-1]
        number = float(raw[:-1])
        if suffix in suffix_map:
            return number * suffix_map[suffix]
        lower = suffix.lower()
        if lower in suffix_map:
            return number * suffix_map[lower]
        raise ValueError(f"unknown suffix: {suffix}")

    return float(raw)


def parse_netlist(netlist_text: str):
    elements = []
    for line in netlist_text.splitlines():
        line = line.strip()
        if not line or line.startswith("*"):
            continue
        if line.lower().startswith(".end"):
            break

        parts = line.split()
        if len(parts) != 4:
            raise ValueError(f"bad line: {line}")

        name = parts[0]
        etype = name[0]
        node_pos = int(parts[1])
        node_neg = int(parts[2])
        value = parse_value(parts[3])
        elements.append((name, etype, node_pos, node_neg, value))

    if not elements:
        raise ValueError("empty circuit")

    max_node = 0
    voltage_sources = []
    for name, etype, node_pos, node_neg, _ in elements:
        max_node = max(max_node, node_pos, node_neg)
        if etype == "V":
            voltage_sources.append(name)

    return elements, max_node, voltage_sources


def solve_linear_system(a, b):
    n = len(a)
    aug = [row[:] + [b[i]] for i, row in enumerate(a)]

    for pivot_col in range(n):
        pivot_row = max(range(pivot_col, n), key=lambda r: abs(aug[r][pivot_col]))
        if abs(aug[pivot_row][pivot_col]) < 1e-15:
            raise ValueError("singular matrix")
        if pivot_row != pivot_col:
            aug[pivot_col], aug[pivot_row] = aug[pivot_row], aug[pivot_col]

        pivot = aug[pivot_col][pivot_col]
        for col in range(pivot_col, n + 1):
            aug[pivot_col][col] /= pivot

        for row in range(pivot_col + 1, n):
            factor = aug[row][pivot_col]
            if factor == 0:
                continue
            for col in range(pivot_col, n + 1):
                aug[row][col] -= factor * aug[pivot_col][col]

    x = [0.0] * n
    for row in range(n - 1, -1, -1):
        x[row] = aug[row][n] - sum(aug[row][col] * x[col] for col in range(row + 1, n))

    return x


def solve_mna(netlist_text: str):
    elements, max_node, voltage_sources = parse_netlist(netlist_text)
    n = max_node
    m = len(voltage_sources)
    size = n + m

    a = [[0.0 for _ in range(size)] for _ in range(size)]
    z = [0.0 for _ in range(size)]
    v_index = {name: idx for idx, name in enumerate(voltage_sources)}

    for name, etype, node_pos, node_neg, value in elements:
        if etype == "R":
            g = 1.0 / value
            if node_pos != 0:
                a[node_pos - 1][node_pos - 1] += g
            if node_neg != 0:
                a[node_neg - 1][node_neg - 1] += g
            if node_pos != 0 and node_neg != 0:
                a[node_pos - 1][node_neg - 1] -= g
                a[node_neg - 1][node_pos - 1] -= g

        elif etype == "I":
            if node_pos != 0:
                z[node_pos - 1] -= value
            if node_neg != 0:
                z[node_neg - 1] += value

        elif etype == "V":
            k = v_index[name]
            row_col = n + k
            z[row_col] = value
            if node_pos != 0:
                a[node_pos - 1][row_col] += 1.0
                a[row_col][node_pos - 1] += 1.0
            if node_neg != 0:
                a[node_neg - 1][row_col] -= 1.0
                a[row_col][node_neg - 1] -= 1.0
        else:
            raise ValueError(f"unsupported element type: {etype}")

    x = solve_linear_system(a, z)
    node_voltages = {f"v{i}": x[i - 1] for i in range(1, max_node + 1)}
    source_currents = {name: x[n + idx] for idx, name in enumerate(voltage_sources)}
    return node_voltages, source_currents


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


def run_cspice_once(netlist_text: str):
    NETLIST_PATH.write_text(netlist_text, encoding="ascii")
    completed = subprocess.run(
        [str(CSPICE_EXE)],
        input="\n",
        capture_output=True,
        text=True,
        cwd=WORKSPACE,
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"cspice exited with code {completed.returncode}\nstdout:\n{completed.stdout}\nstderr:\n{completed.stderr}"
        )

    node_pattern = re.compile(r"^v(\d+)\s*=\s*([-+]?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?)\s+V$", re.MULTILINE)
    source_pattern = re.compile(r"^I\((V\w+)\)\s*=\s*([-+]?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?)\s+A$", re.MULTILINE)

    node_voltages = {f"v{match.group(1)}": float(match.group(2)) for match in node_pattern.finditer(completed.stdout)}
    source_currents = {match.group(1): float(match.group(2)) for match in source_pattern.finditer(completed.stdout)}
    return node_voltages, source_currents, completed.stdout


def run_case(netlist_path: Path):
    netlist_text = netlist_path.read_text(encoding="ascii")
    expected_v, expected_i = solve_mna(netlist_text)
    actual_v, actual_i, raw_output = run_cspice_once(netlist_text)

    v_ok, v_max_err, v_rows = compare_maps(expected_v, actual_v)
    i_ok, i_max_err, i_rows = compare_maps(expected_i, actual_i)
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
        print("\nRaw cspice output for debugging:")
        print(raw_output)

    print()
    return passed


def resolve_suite_dir(arg: str | None):
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


def main():
    if not CSPICE_EXE.exists():
        raise FileNotFoundError(f"missing executable: {CSPICE_EXE}")

    suite_dir = resolve_suite_dir(sys.argv[1] if len(sys.argv) > 1 else None)
    if not suite_dir.exists():
        raise FileNotFoundError(f"missing suite directory: {suite_dir}")

    netlists = sorted(suite_dir.glob("*.sp"))
    if not netlists:
        raise FileNotFoundError(f"no .sp files found in {suite_dir}")

    overall = True
    for netlist_path in netlists:
        overall = run_case(netlist_path) and overall

    print("=" * 80)
    print(f"OVERALL: {GREEN}PASS{RESET}" if overall else f"OVERALL: {RED}FAIL{RESET}")


if __name__ == "__main__":
    main()
