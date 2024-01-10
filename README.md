
# Linkage Editor for ELF Files

## Description:
This project implements a partial linkage editor operation for ELF (Executable and Linkable Format) files using C. The focus lies in leveraging mmap system calls for efficient data handling, parsing crucial ELF structures like section headers and symbol tables.

## Features:
- **Linkage Editor:** Implementation of limited pass I operation resembling a linkage editor for ELF files.
- **Efficient Data Handling:** Utilization of mmap system calls for optimized data access and manipulation.
- **Debugging Tools Integration:** Use of hexedit and readelf for insights and debugging during development.

## How to Use:
1. Clone the repository: `git clone https://github.com/Ohad-Amzaleg/Linkage-Editor-for-ELF-Files.git`
2. Navigate to the project directory.
3. Compile the C files using the `-m32` flag to ensure compatibility with 32-bit systems.
    ```
    $ gcc -m32 -o partial_editor partial_editor.c
    ```
4. Execute the compiled program.
    ```
    $ ./partial_editor <elf_file_1> <elf_file_2>
    ```
