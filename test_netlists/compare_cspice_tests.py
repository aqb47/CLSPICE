import math
import re
import subprocess
import sys
from pathlib import Path

WORKSPACE = Path(__file__).resolve().parent
NETLIST_PATH = WORKSPACE / "netlist.sp"
CSPICE_EXE = WORKSPACE / "cspice.exe"

TOLERANCE = 1e-6

GOLDEN_SUITE_DIR = WORKSPACE / "test_netlists" / "golden"
DEPENDENT_SUITE_DIR = WORKSPACE / "test_netlists" / "dependent_sources"

TEST_CASES = {
    "case1_bridge_multisource": """* Case 1: bridge network with mixed sources
V1 1 0 18
V2 4 2 5
I1 3 0 2m
I2 0 5 1m
R1 1 2 1k
R2 2 3 2.2k
R3 3 0 1.5k
R4 1 3 3.3k
R5 2 4 4.7k
R6 4 5 2k
R7 5 0 1.2k
R8 3 5 5.6k
R9 1 4 8.2k
.end
""",
    "case2_ladder_coupled": """* Case 2: ladder and cross-coupled branches
V1 1 0 24
V2 6 3 7
I1 2 0 3m
I2 0 4 1.2m
I3 5 1 0.8m
R1 1 2 1.8k
R2 2 3 2.7k
R3 3 0 1k
R4 1 4 3.9k
R5 4 5 2.2k
R6 5 0 4.7k
R7 2 5 6.8k
R8 3 6 1.5k
R9 6 0 2.4k
R10 4 6 3.3k
.end
""",
    "case3_three_sources": """* Case 3: three voltage sources and distributed current injections
V1 2 0 12
V2 5 4 9
V3 7 1 -4
I1 0 3 2.5m
I2 6 0 1.1m
R1 1 2 2k
R2 2 3 1.5k
R3 3 4 3.3k
R4 4 0 2.2k
R5 1 5 4.7k
R6 5 6 1.2k
R7 6 0 3.9k
R8 3 6 5.6k
R9 2 7 2.8k
R10 7 0 1.1k
R11 4 7 6.2k
.end
""",
    "case4_seven_node_dense": """* Case 4: dense 7-node test with multiple meshes
V1 1 0 15
V2 4 2 6
V3 7 5 4
I1 0 3 1m
I2 5 0 2.2m
I3 6 1 0.7m
R1 1 2 1k
R2 2 3 2k
R3 3 0 1.5k
R4 1 4 3k
R5 4 5 4k
R6 5 0 2.2k
R7 2 5 5k
R8 3 6 3.3k
R9 6 0 2.7k
R10 4 7 1.8k
R11 7 0 2.4k
R12 6 7 4.6k
R13 5 7 6.8k
.end
""",
}


def list_suite_files(suite_dir: Path):
    return sorted(suite_dir.glob("*.sp"))


def parse_value(raw: str) -> float:
    raw = raw.strip()
    if not raw:
        raise ValueError("empty value")

    suffix_map = {
        "u": 1e-6,
        "m": 1e-3,
        "k": 1e3,
        "M": 1e6,
        "g": 1e9,
        "t": 1e12,
    }

    suffix = raw[-1]
    if suffix.isalpha():
        number = float(raw[:-1])
        if suffix not in suffix_map:
            lower = suffix.lower()
            if lower in suffix_map:
                number *= suffix_map[lower]
                return number
            raise ValueError(f"unknown suffix: {suffix}")
        return number * suffix_map[suffix]

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

    stdout = completed.stdout

    node_pattern = re.compile(r"^v(\d+)\s*=\s*([-+]?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?)\s+V$", re.MULTILINE)
    vsrc_pattern = re.compile(r"^I\((V\w+)\)\s*=\s*([-+]?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?)\s+A$", re.MULTILINE)

    node_voltages = {f"v{m.group(1)}": float(m.group(2)) for m in node_pattern.finditer(stdout)}
    source_currents = {m.group(1): float(m.group(2)) for m in vsrc_pattern.finditer(stdout)}

    return node_voltages, source_currents, stdout


def run_file_case(netlist_path: Path):
    netlist_text = netlist_path.read_text(encoding="ascii")
    expected_v, expected_i = solve_mna(netlist_text)
    actual_v, actual_i, raw_out = run_cspice_once(netlist_text)

    v_ok, v_max_err, v_rows = compare_maps("Node Voltages", expected_v, actual_v, TOLERANCE)
    i_ok, i_max_err, i_rows = compare_maps("Voltage Source Currents", expected_i, actual_i, TOLERANCE)

    case_pass = v_ok and i_ok
    print("=" * 80)
    print(f"TEST: {netlist_path.stem}")
    print(f"RESULT: {'PASS' if case_pass else 'FAIL'}")
    print(f"Max |dv|: {v_max_err:.3e} V")
    print(f"Max |di|: {i_max_err:.3e} A")

    print("\nNode voltages:")
    for key, exp, act, err, ok in v_rows:
        if exp is None or act is None:
            print(f"  {key:>5}: expected={exp} actual={act} -> MISSING")
        else:
            print(f"  {key:>5}: expected={exp:+.9f} actual={act:+.9f} |err|={err:.3e} {'OK' if ok else 'BAD'}")

    print("\nVoltage source currents:")
    for key, exp, act, err, ok in i_rows:
        if exp is None or act is None:
            print(f"  {key:>5}: expected={exp} actual={act} -> MISSING")
        else:
            print(f"  {key:>5}: expected={exp:+.9f} actual={act:+.9f} |err|={err:.3e} {'OK' if ok else 'BAD'}")

    if not case_pass:
        print("\nRaw cspice output for debugging:")
        print(raw_out)

    print()
    return case_pass


def compare_maps(label, expected, actual, tolerance):
    all_keys = sorted(set(expected) | set(actual), key=lambda x: (x[0], int(x[1:]) if x[1:].isdigit() else x))
    rows = []
    max_abs_err = 0.0

    for key in all_keys:
        exp = expected.get(key)
        act = actual.get(key)

        if exp is None or act is None:
            rows.append((key, exp, act, None, False))
            continue

        err = abs(exp - act)
        ok = err <= tolerance
        rows.append((key, exp, act, err, ok))
        max_abs_err = max(max_abs_err, err)

    passed = all(r[4] for r in rows)
    return passed, max_abs_err, rows


def main():
    if not CSPICE_EXE.exists():
        raise FileNotFoundError(f"missing executable: {CSPICE_EXE}")

    if len(sys.argv) > 1:
        requested_dir = Path(sys.argv[1])
        if not requested_dir.is_absolute():
            requested_dir = WORKSPACE / requested_dir

        if not requested_dir.exists():
            raise FileNotFoundError(f"missing suite directory: {requested_dir}")

        suite_files = list_suite_files(requested_dir)
        if not suite_files:
            raise FileNotFoundError(f"no .sp files found in suite directory: {requested_dir}")

        overall_pass = True
        for netlist_path in suite_files:
            overall_pass = run_file_case(netlist_path) and overall_pass

        print("=" * 80)
        print(f"OVERALL: {'PASS' if overall_pass else 'FAIL'}")
        return

    overall_pass = True

    for case_name, netlist in TEST_CASES.items():
        expected_v, expected_i = solve_mna(netlist)
        actual_v, actual_i, raw_out = run_cspice_once(netlist)

        v_ok, v_max_err, v_rows = compare_maps("Node Voltages", expected_v, actual_v, TOLERANCE)
        i_ok, i_max_err, i_rows = compare_maps("Voltage Source Currents", expected_i, actual_i, TOLERANCE)

        case_pass = v_ok and i_ok
        overall_pass = overall_pass and case_pass

        print("=" * 80)
        print(f"TEST: {case_name}")
        print(f"RESULT: {'PASS' if case_pass else 'FAIL'}")
        print(f"Max |dv|: {v_max_err:.3e} V")
        print(f"Max |di|: {i_max_err:.3e} A")

        print("\nNode voltages:")
        for key, exp, act, err, ok in v_rows:
            if exp is None or act is None:
                print(f"  {key:>5}: expected={exp} actual={act} -> MISSING")
            else:
                print(f"  {key:>5}: expected={exp:+.9f} actual={act:+.9f} |err|={err:.3e} {'OK' if ok else 'BAD'}")

        print("\nVoltage source currents:")
        for key, exp, act, err, ok in i_rows:
            if exp is None or act is None:
                print(f"  {key:>5}: expected={exp} actual={act} -> MISSING")
            else:
                print(f"  {key:>5}: expected={exp:+.9f} actual={act:+.9f} |err|={err:.3e} {'OK' if ok else 'BAD'}")

        if not case_pass:
            print("\nRaw cspice output for debugging:")
            print(raw_out)

        print()

    print("=" * 80)
    print(f"OVERALL: {'PASS' if overall_pass else 'FAIL'}")


if __name__ == "__main__":
    main()
