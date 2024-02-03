# bin2c.py
# Convert binary files to header files for C and C++
import sys

if len(sys.argv) != 3:
    print("Usage: python bin2c.py input_binary output_c")
    sys.exit(1)

input_binary = sys.argv[1]
output_c = sys.argv[2]

with open(input_binary, 'rb') as binary_file, open(output_c, 'w') as c_file:
    binary_data = binary_file.read()
    hex_data = ', '.join(f'0x{byte:02x}' for byte in binary_data)
    c_file.write(f"unsigned char {output_c[:-2]}[] = {{ {hex_data} }};\n")
