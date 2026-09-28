# Usage: python generateFile.py <output_filename> <size_in_mb>
# Description: Generate a file of given size in MB with random data.
# Create by Microsoft CoPilot, July 30, 2024
import sys
import os
import random
import string

def generate_random_line(length=70):
    return ''.join(random.choices(string.printable.strip(), k=length))

def generate_file(filename, size_mb):
    size_bytes = size_mb * 1024
    # size_bytes = size_mb * 1024 * 1024
    with open(filename, 'w') as f:
        while os.path.getsize(filename) < size_bytes:
            line = generate_random_line() + '\n'
            f.write(line)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python script.py <output_filename> <size_in_mb>")
        sys.exit(1)

    output_filename = sys.argv[1]
    size_in_mb = int(sys.argv[2])

    generate_file(output_filename, size_in_mb)
    print(f"File '{output_filename}' of size {size_in_mb} MB generated successfully.")
