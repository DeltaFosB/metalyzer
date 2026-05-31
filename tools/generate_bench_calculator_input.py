#!/usr/bin/env python3
import argparse
import os
import random


def generate_calculator_dense():
    # Dense mode: Compact, stacked tokens with minimal to no optional whitespace
    # to maximize allocation and state transition churn.
    expr_type = random.choice(["assign", "compound", "paren_expr"])
    var_name = "".join(
        random.choices("abcdefghijklmnopqrstuvwxyz", k=random.randint(1, 6))
    )
    num1 = "".join(random.choices("0123456789", k=random.randint(1, 4)))
    num2 = "".join(random.choices("0123456789", k=random.randint(1, 4)))
    op1 = random.choice(["+", "-", "*", "/"])

    if expr_type == "assign":
        return f"{var_name}={num1}"
    elif expr_type == "compound":
        return f"{var_name}+={num1}{op1}{num2}"
    else:
        return f"({var_name}{op1}{num1})/{num2}"


def generate_calculator_sparse():
    # Sparse mode: Valid calculator sequences broken up by massive blocks
    # of raw whitespace, deeply nested tabs, and newline sequences to hammer the skipper loops.
    var_name = "".join(
        random.choices("abcdefghijklmnopqrstuvwxyz", k=random.randint(2, 5))
    )
    num = "".join(random.choices("0123456789", k=random.randint(1, 3)))

    # Random mix of spacing layouts
    spaces = " " * random.randint(10, 80)
    tabs = "\t" * random.randint(4, 16)
    newlines = "\n" * random.randint(2, 8)

    return f"{var_name}{spaces}={tabs}{num}{newlines}"


def generate_calculator_error():
    # Error Churn mode: Spikes valid math statements with illegal control and
    # punctuation symbols to continuously trigger single-byte stream rollbacks and error states.
    var_name = "".join(
        random.choices("abcdefghijklmnopqrstuvwxyz", k=random.randint(2, 4))
    )
    num = "".join(random.choices("0123456789", k=random.randint(1, 4)))

    # Inject foreign lexical symbols ($ , @ , # , _ , { , } , [ , ])
    bad_symbol = random.choice(["@", "#", "$", "_", "{", "}", "[", "]", "!", "?"])
    err_pattern = random.choice(
        [
            f"{var_name}{bad_symbol}={num}",  # Split identifier
            f"{var_name}={bad_symbol}{num}",  # Leading literal error
            f"{var_name}+{bad_symbol}=={num}",  # Compounded illegal operators
        ]
    )
    return err_pattern


def generate_benchmark_file(target_filename, target_size_mb, mode):
    target_bytes = target_size_mb * 1024 * 1024
    generated_bytes = 0

    print("=================================================================")
    print("METALYSER ENGINE HARDWARE GENERATION LABORATORY")
    print("=================================================================")
    print(f"[*] Target File:  {target_filename}")
    print("[*] Profile:      CALCULATOR Grammar Subsystem")
    print(f"[*] Layout Mode:  {mode.upper()}")
    print(f"[*] Weight Scale: {target_size_mb:.2f} MB")
    print("[*] Streaming data chunks... ", end="", flush=True)

    with open(target_filename, "w") as f:
        line_buffer = []

        while generated_bytes < target_bytes:
            if mode == "dense":
                line = generate_calculator_dense()
            elif mode == "sparse":
                line = generate_calculator_sparse()
            else:
                line = generate_calculator_error()

            line_buffer.append(line)

            # Flush blocks to disk periodically to control heap memory footprint
            if len(line_buffer) >= 5000:
                chunk = "\n".join(line_buffer) + "\n"
                f.write(chunk)
                generated_bytes += len(chunk.encode("utf-8"))
                line_buffer = []

        if line_buffer:
            chunk = "\n".join(line_buffer) + "\n"
            f.write(chunk)
            generated_bytes += len(chunk.encode("utf-8"))

    actual_size = os.path.getsize(target_filename) / (1024 * 1024)
    print("Done.")
    print(f"[+] Output Verification Size: {actual_size:.2f} MB\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Calculator Grammar High-Volume Benchmark Payload Generator"
    )
    parser.add_argument(
        "--mode",
        choices=["dense", "sparse", "error"],
        required=True,
        help="Select allocation density profile",
    )
    parser.add_argument(
        "--output", required=True, help="Target output data payload path"
    )
    parser.add_argument(
        "--size", type=int, default=10, help="Payload allocation weight target in MB"
    )

    args = parser.parse_args()

    # Ensure directory existence before streaming file
    output_dir = os.path.dirname(args.output)
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)

    generate_benchmark_file(args.output, args.size, args.mode)
