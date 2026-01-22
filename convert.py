# /// script
# requires-python = ">=3.12"
# dependencies = [
#     "numpy",
#     "scipy",
# ]
# ///

import os
import json
import argparse
import numpy as np
import scipy.io as sio
import scipy.sparse

def load_mat_file(filepath):
    try:
        mat = sio.loadmat(filepath)
        return mat
    except Exception as e:
        print(f"Error loading {filepath}: {e}")
        return None

def convert_to_list(array):
    if scipy.sparse.issparse(array):
        array = array.toarray()
    if isinstance(array, np.ndarray):
        return array.tolist()
    return array

def handle_infinity(val, large_val=1e20):
    # If val is list of lists
    if isinstance(val, list):
        return [handle_infinity(item, large_val) for item in val]
    # If val is float or int
    if isinstance(val, (float, int, np.floating, np.integer)):
        if np.isinf(val):
            return large_val if val > 0 else -large_val
        return val
    return val

def check_positive_definite(matrix):
    # Check if symmetric
    if not np.allclose(matrix, matrix.T, atol=1e-8):
        return False
    # Check eigenvalues
    try:
        eigenvalues = np.linalg.eigvalsh(matrix)
        return np.min(eigenvalues) > 1e-13
    except np.linalg.LinAlgError:
        return False

def extract_variable(mat, names, default=None):
    for name in names:
        if name in mat:
            var = mat[name]
            # Flatten vectors if they are 2D arrays (n, 1) or (1, n)
            if isinstance(var, np.ndarray):
                if var.ndim == 2 and (var.shape[0] == 1 or var.shape[1] == 1):
                    var = var.flatten()
            return var
    return default

def to_json_str(obj, level=0):
    indent = "\t" * level
    if isinstance(obj, dict):
        items = []
        for k, v in obj.items():
            items.append(f'{indent}\t"{k}": {to_json_str(v, level+1)}')
        return "{\n" + ",\n".join(items) + f"\n{indent}}}"
    elif isinstance(obj, list):
        # Check if it's a list of numbers (simple)
        if obj and all(isinstance(x, (int, float)) for x in obj):
            # Compact list
            return json.dumps(obj)
        elif obj and all(isinstance(x, list) and all(isinstance(y, (int, float)) for y in x) for x in obj):
            # List of lists of numbers (matrix)
            items = []
            for item in obj:
                items.append(f'{indent}\t{json.dumps(item)}')
            return "[\n" + ",\n".join(items) + f"\n{indent}]"
        else:
            # General list
            items = []
            for item in obj:
                items.append(f'{indent}\t{to_json_str(item, level+1)}')
            return "[\n" + ",\n".join(items) + f"\n{indent}]"
    else:
        return json.dumps(obj)

def convert_mat_to_json(mat_path, output_dir):
    filename = os.path.basename(mat_path)
    base_name = os.path.splitext(filename)[0]
    json_path = os.path.join(output_dir, base_name + '.json')

    mat = load_mat_file(mat_path)
    if mat is None:
        return

    # Extract variables with common names in Maros Meszaros dataset
    # Hessian
    H = extract_variable(mat, ['Q', 'H'])
    if H is None:
        print(f"Skipping {filename}: Hessian (Q/H) not found.")
        return

    # Gradient/Objective vector
    g = extract_variable(mat, ['c', 'g', 'f'])
    if g is None:
        # Assume zero if dimensions match H
        if scipy.sparse.issparse(H):
            n = H.shape[0]
        else:
            n = len(H)
        g = np.zeros(n)
        print(f"Warning {filename}: Gradient (c/g) not found, assuming zeros.")

    # Constraints Matrix
    A = extract_variable(mat, ['A'])

    # Constraint bounds
    rl = extract_variable(mat, ['rl', 'lhs', 'l', 'lower'])
    ru = extract_variable(mat, ['ru', 'rhs', 'u', 'upper'])

    # Variable bounds
    lb = extract_variable(mat, ['lb', 'xmin'])
    ub = extract_variable(mat, ['ub', 'xmax'])

    # Solution (optional)
    x = extract_variable(mat, ['x', 'sol', 'z'])

    # Normalize dimensions
    if scipy.sparse.issparse(H):
        n = H.shape[0]
        H_dense = H.toarray()
    else:
        H_dense = np.array(H)
        n = H_dense.shape[0]

    # Ensure g is length n
    if len(g) != n:
        print(f"Warning {filename}: Gradient dimension mismatch. Expected {n}, got {len(g)}.")
        return

    # Process A
    has_constraints = False
    if A is not None:
        if scipy.sparse.issparse(A):
            m = A.shape[0]
            A_dense = A.toarray()
        else:
            A_dense = np.array(A)
            if A_dense.size > 0:
                m = A_dense.shape[0]
            else:
                m = 0

        if m > 0:
            has_constraints = True
            # Check rl and ru dimensions
            if rl is None: rl = np.full(m, -np.inf)
            if ru is None: ru = np.full(m, np.inf)

            if len(rl) != m or len(ru) != m:
                print(f"Warning {filename}: Constraint bounds dimension mismatch.")
        else:
            A_dense = None
    else:
        A_dense = None

    # Process bounds
    if lb is None: lb = np.full(n, -np.inf)
    if ub is None: ub = np.full(n, np.inf)

    # Check Positive Definite
    is_pd = check_positive_definite(H_dense)

    # Construct JSON structure
    qp_data = {
        "objective": {
            "hessian": convert_to_list(H_dense),
            "vector": convert_to_list(g),
            "positive_definite": bool(is_pd)
        },
        "bounds": {
            "lower": convert_to_list(handle_infinity(lb)),
            "upper": convert_to_list(handle_infinity(ub))
        }
    }

    if has_constraints:
        qp_data["constraints"] = {
            "matrix": convert_to_list(A_dense),
            "lower": convert_to_list(handle_infinity(rl)),
            "upper": convert_to_list(handle_infinity(ru))
        }

    if x is not None:
        # Calculate objective value if possible
        x_vec = np.array(x).flatten()
        if len(x_vec) == n:
            val = 0.5 * x_vec.T @ H_dense @ x_vec + g.T @ x_vec
            qp_data["solution"] = {
                "vector": convert_to_list(x_vec),
                "value": float(val)
            }

    json_output = {"qp": qp_data}

    try:
        with open(json_path, 'w') as f:
            f.write(to_json_str(json_output))
        print(f"Converted {filename} to {json_path}")
    except Exception as e:
        print(f"Error writing JSON for {filename}: {e}")

def main():
    parser = argparse.ArgumentParser(description="Convert Maros Meszaros .mat files to JSON.")
    parser.add_argument("input_dir", help="Directory containing .mat files")
    parser.add_argument("output_dir", help="Directory to save JSON files")

    args = parser.parse_args()

    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)

    if not os.path.exists(args.input_dir):
        print(f"Input directory {args.input_dir} does not exist.")
        return

    files = [f for f in os.listdir(args.input_dir) if f.endswith('.mat')]
    if not files:
        print(f"No .mat files found in {args.input_dir}.")
        return

    print(f"Found {len(files)} .mat files. Starting conversion...")
    for f in files:
        convert_mat_to_json(os.path.join(args.input_dir, f), args.output_dir)

if __name__ == "__main__":
    main()
