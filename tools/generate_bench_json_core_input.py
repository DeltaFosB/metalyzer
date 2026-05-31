#!/usr/bin/env python3
import argparse
import os
import random


def generate_json_dense():
    # Dense mode: Highly compact nested JSON structures with no optional spacing.
    # Forces rapid key/value state changes and string literal heap constructions.
    structure_type = random.choice(["object", "array", "flat_pairs"])
    keys = ["id", "index", "status", "payload", "enabled", "type", "tag", "meta"]
    vals = ["true", "false", "null", "100", '"active"', '"production"', '"verified"']

    if structure_type == "object":
        k1, k2 = random.sample(keys, 2)
        v1 = random.choice(vals)
        # Inject string literals with a mix of normal text and escaped sequences
        str_lit = '"val_\\n_\\t_\\\\_\\""'
        return f'{{"{k1}":{v1},"{k2}":{str_lit}}}'

    elif structure_type == "array":
        v1 = random.choice(vals)
        v2 = random.choice(vals)
        return f'[{v1},{v2},["nested",true]]'

    else:
        k1 = random.choice(keys)
        return f'"{k1}":{random.choice(vals)}'


def generate_json_sparse():
    # Sparse mode: Spreads valid structural brackets and properties across
    # massive blocks of tab indents and tracking carriage returns to strain the skipper.
    structure = random.choice(["{", "}", "[", "]", ":", ","])

    spaces = " " * random.randint(15, 60)
    tabs = "\t" * random.randint(5, 15)
    newlines = "\n" * random.randint(3, 10)

    return f"{structure}{spaces}{newlines}{tabs}"


def generate_json_error():
    # Error Churn mode: Emits invalid structures, bad escape formats, and
    # unquoted tokens to continuously hammer the rollback/putback architecture.
    err_type = random.choice(["unquoted", "bad_escape", "broken_string"])

    if err_type == "unquoted":
        # Missing quotes around dictionary property names
        return '{illegal_key:true,type:"json"}'
    elif err_type == "bad_escape":
        # Invalid hexadecimal or letter escape sequence inside strings
        return '{"key":"value_\\x_bad_escape"}'
    else:
        # String literal that never closes, or contains raw unescaped newlines
        return '{"missing_quote":"unterminated'


def generate_benchmark_file(target_filename, target_size_mb, mode):
    target_bytes = target_size_mb * 1024 * 1024
    generated_bytes = 0

    print("=================================================================")
    print("METALYSER ENGINE HARDWARE GENERATION LABORATORY")
    print("=================================================================")
    print(f"[*] Target File:  {target_filename}")
    print("[*] Profile:      JSON_CORE Grammar Subsystem")
    print(f"[*] Layout Mode:  {mode.upper()}")
    print(f"[*] Weight Scale: {target_size_mb:.2f} MB")
    print("[*] Streaming data chunks... ", end="", flush=True)

    with open(target_filename, "w") as f:
        line_buffer = []

        while generated_bytes < target_bytes:
            if mode == "dense":
                line = generate_json_dense()
            elif mode == "sparse":
                line = generate_json_sparse()
            else:
                line = generate_json_error()

            line_buffer.append(line)

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
        description="JSON Core Grammar High-Volume Benchmark Payload Generator"
    )
    parser.add_argument("--mode", choices=["dense", "sparse", "error"], required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--size", type=int, default=10)

    args = parser.parse_args()
    output_dir = os.path.dirname(args.output)
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)

    generate_benchmark_file(args.output, args.size, args.mode)
