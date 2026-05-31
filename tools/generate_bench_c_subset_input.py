#!/usr/bin/env python3
import argparse
import os
import random


def generate_c_subset_dense():
    # Dense mode: Concentrated keyword declarations, overlapping identifier tags,
    # and compound symbols packed tightly together with zero formatting paddings.
    block_type = random.choice(["decl", "condition", "loop_step"])
    types = ["int", "float", "double", "char", "unsigned", "static", "volatile"]
    keywords = ["return", "break", "continue", "sizeof", "struct", "typedef"]

    var1 = "".join(
        random.choices("abcdefghijklmnopqrstuvwxyz_", k=random.randint(3, 8))
    )
    var2 = "".join(
        random.choices("abcdefghijklmnopqrstuvwxyz_", k=random.randint(3, 8))
    )

    if block_type == "decl":
        t = random.choice(types)
        return f"{t} {var1}=12.34;{random.choice(keywords)} {var1};"

    elif block_type == "condition":
        op = random.choice(["==", "!=", "<=", ">=", "&&", "||"])
        return f"if({var1}{op}{var2}){{{var1}->{var2}=10;}}else{{{var1}++;}}"

    else:
        return f"for({var1}=0;{var1}<100;{var1}++){{{var2}--=sizeof({var1});}}"


def generate_c_subset_sparse():
    # Sparse mode: Intersperses small token clusters with massive multi-line block comments
    # (/* ... */), spacing arrays, and trailing line feeds to test comment skipper efficiency.
    sparse_type = random.choice(["spaces", "block_comment"])
    token = random.choice(["int", "while", "return", "++", "->", ";", "{", "}"])

    if sparse_type == "spaces":
        spaces = " " * random.randint(20, 80)
        tabs = "\t" * random.randint(4, 12)
        newlines = "\n" * random.randint(4, 10)
        return f"{token}{spaces}{newlines}{tabs}"
    else:
        # Generate a large multi-line block comment filler
        comment_lines = []
        for _ in range(random.randint(2, 6)):
            comment_lines.append(
                " * " + "".join(random.choices("abcdefghijklmnopqrstuvwxyz ", k=40))
            )
        comment_block = "/*\n" + "\n".join(comment_lines) + "\n */"
        return f"{token}\n{comment_block}\n"


def generate_c_subset_error():
    # Error Churn mode: Splices unrecognized symbols straight into the center of
    # keywords or operators, triggering massive single-byte lookahead rollbacks.
    var_name = "".join(
        random.choices("abcdefghijklmnopqrstuvwxyz", k=random.randint(3, 6))
    )
    bad_char = random.choice(["@", "#", "$", "`", "?", "\\", "^"])

    err_pattern = random.choice(
        [
            f"unsig{bad_char}ned int x=0;",  # Broken structural keyword
            f"if(x +{bad_char}+ y)",  # Broken compound operator target
            f"struct {var_name} {{ int {bad_char}field; }};",  # Illegal symbol prefix
            f'char* s = "valid_string{bad_char}',  # Unterminated literal boundary
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
    print("[*] Profile:      C_SUBSET Grammar Subsystem")
    print(f"[*] Layout Mode:  {mode.upper()}")
    print(f"[*] Weight Scale: {target_size_mb:.2f} MB")
    print("[*] Streaming data chunks... ", end="", flush=True)

    with open(target_filename, "w") as f:
        line_buffer = []

        while generated_bytes < target_bytes:
            if mode == "dense":
                line = generate_c_subset_dense()
            elif mode == "sparse":
                line = generate_c_subset_sparse()
            else:
                line = generate_c_subset_error()

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
        description="C Subset Grammar High-Volume Benchmark Payload Generator"
    )
    parser.add_argument("--mode", choices=["dense", "sparse", "error"], required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--size", type=int, default=10)

    args = parser.parse_args()
    output_dir = os.path.dirname(args.output)
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)

    generate_benchmark_file(args.output, args.size, args.mode)
