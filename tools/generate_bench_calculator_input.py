#!/usr/bin/env python3
import random
import os


def generate_benchmark_file(target_filename, target_size_mb):
    operators = ["+", "-", "*", "/", "=", "+="]
    parens = ["(", ")"]

    target_bytes = target_size_mb * 1024 * 1024
    generated_bytes = 0

    print("[*] Initializing high-volume data stream generation...")
    print(f"[*] Target: {target_filename} ({target_size_mb} MB)")

    with open(target_filename, "w") as f:
        line_buffer = []

        while generated_bytes < target_bytes:
            expr_type = random.choice(["assignment", "arithmetic", "paren_block"])

            if expr_type == "assignment":
                var_name = "".join(
                    random.choices("abcdefghijklmnopqrstuvwxyz", k=random.randint(1, 8))
                )
                op = random.choice(["=", "+="])
                num = "".join(random.choices("0123456789", k=random.randint(1, 5)))
                line = f"{var_name} {op} {num}"

            elif expr_type == "arithmetic":
                term1 = "".join(random.choices("0123456789", k=random.randint(1, 4)))
                op = random.choice(["+", "-", "*", "/"])
                term2 = "".join(
                    random.choices("abcdefghijklmnopqrstuvwxyz", k=random.randint(2, 6))
                )
                line = f"{term1} {op} {term2}"

            else:
                var_name = "".join(
                    random.choices("abcdefghijklmnopqrstuvwxyz", k=random.randint(3, 7))
                )
                op = random.choice(["+", "-", "*", "/"])
                num = "".join(random.choices("0123456789", k=random.randint(1, 4)))
                line = f"({var_name} {op} {num})"

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
    print("[+] Success! Benchmark input generated.")
    print(f"    File path:   {target_filename}")
    print(f"    Actual size: {actual_size:.2f} MB")


if __name__ == "__main__":
    output_dir = "tests/inputs"
    os.makedirs(output_dir, exist_ok=True)

    target_file = os.path.join(output_dir, "bench_calculator_input.txt")
    generate_benchmark_file(target_file, target_size_mb=10)
