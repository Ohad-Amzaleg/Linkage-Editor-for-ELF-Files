#ifndef ELF_FILE_H
#define ELF_FILE_H

#include <elf.h>
#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  Elf32_Shdr *section_header;
  GHashTable *section_map;
} SECTION_DATA;

typedef struct {
  Elf32_Sym *symbol_table;
  GHashTable *symbol_map;
  int symbol_count;
} SYMBOL_DATA;

typedef struct {
  Elf32_Ehdr *header;
  SECTION_DATA *sections;
  SYMBOL_DATA *symbols;
} ELF_FILE;

typedef struct {
  char *file_name;
  int fd;
  ELF_FILE *elf_file;
  int file_size;
} FILE_DATA;

FILE_DATA *file_data_init(char *filename);
void *elf_file_init(char *filename);
void elf_file_free(ELF_FILE *elf_file);
void elf_file_print_sections();
void elf_file_print_symbols();
void elf_file_print_header();
void init_section_data(ELF_FILE *elf_file);
void init_symbol_data(ELF_FILE *elf_file);

#endif
