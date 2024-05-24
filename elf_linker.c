#include "logger.c"
#include <elf_file.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
//---------------- General ELF File Structure ----------------
// ELF Header
// ELF FIELDS
// 1. e_ident : ELF Identification
// 2. e_type : Object file type
// 3. e_machine : Machine type
// 4. e_version : Object file version
// 5. e_entry : Entry point address
// 6. e_phoff : Program header Offset
// 7. e_shoff : Section header offset
// 8. e_flags : Processor-specific flags
// 9. e_ehsize : ELF header size
// 10. e_phentsize : Size of program header entry
// 11. e_phnum : Number of program header entries
// 12. e_shentsize : Size of section header entry
// 13. e_shnum : Number of section header entries
// 14. e_shstrndx : Section name string table index
//--------------------------------- Constants ---------------------------------
#define MAX_ELF_FILES 2
#define FILE_1 0
#define FILE_2 1

// --------------------------------- Data Structure
// ---------------------------------

static ELF_FILE *elf_file[MAX_ELF_FILES];
static FILE_DATA *file_data[MAX_ELF_FILES];
static int next_file = 0;
static int current_file = 0;
static int debug = 1;
static FILE *log_file = NULL;
struct menu_desc {
  char *action;
  void (*func)();
};

// ------------------------------------------------
// ---------------------------------
void toggleDebug() {
  debug = !debug;
  printf("Debug mode is now %s\n", debug ? "ON" : "OFF");
}

void elf_load_file() {
  char filename[256];
  printf("Enter the name of the ELF file to load: ");
  scanf("%s", filename);
  elf_file_init(filename);
}

// --------------------------------- Elf File initialization
// ---------------------------------
//@brief : Initialize the file data structure by opening the file and getting
// the file descriptor
// @param file_name : The name of the file to open
// @return : A pointer to the file data structure
FILE_DATA *elf_file_data_init(char *file_name) {
  FILE_DATA *file = malloc(sizeof(FILE_DATA));
  if (!file) {
    log_message(log_file, LOG_ERROR,
                "Failed to allocate memory for file data %s \n", file_name);
    return NULL;
  }
  // Copy the file name and open the file
  file->file_name = strdup(file_name);
  // Open the file and get the file descriptor
  file->fd = open(file_name, O_RDONLY);
  if (file->fd < 0) {
    log_message(log_file, LOG_ERROR, "Failed to open file %s \n", file_name);
    printf("Failed to open file %s \n", file_name);
    free(file);
    return NULL;
  }

  // Get the file size
  file->file_size = lseek(file->fd, 0, SEEK_END);
  if (file->file_size < 0) {
    log_message(log_file, LOG_ERROR, "Failed to get file size %s \n",
                file_name);

    close(file->fd);
    free(file);
    return NULL;
  }

  if (debug) {
    printf("File opened successfully : %s\n", file_name);
  }
  log_message(log_file, LOG_INFO, "File opened successfully : %s\n", file_name);

  return file;
}

//@brief : Initialize the ELF file by opening the file and getting the ELF
// header,
// @param filename : The name of the ELF file to open
// @return : A pointer to the ELF file data structure
void *elf_file_init(char *filename) {
  if (next_file >= MAX_ELF_FILES) {
    printf("Maximum number of ELF files reached\n");
    return NULL;
  }
  file_data[next_file] =
      elf_file_data_init(filename); // Initialize file data for the current file
  if (!file_data[next_file]) {
    return NULL;
  }
  elf_file[next_file] =
      malloc(sizeof(ELF_FILE)); // Allocate memory for the ELF file
  if (!elf_file[next_file]) {
    log_message(log_file, LOG_ERROR,
                "Failed to allocate memory for ELF file\n");
    free(file_data[next_file]);
    return NULL;
  }
  elf_file[next_file]->header =
      mmap(NULL, file_data[next_file]->file_size, PROT_READ, MAP_PRIVATE,
           file_data[next_file]->fd,
           0); // Map the ELF file to memory and get the header

  if (elf_file[next_file]->header == MAP_FAILED) {
    log_message(log_file, LOG_ERROR, "Failed to map ELF file %s\n", filename);
    free(elf_file[next_file]);
    free(file_data[next_file]);
    return NULL;
  }

  if (debug) {
    printf("ELF file mapped successfully : %s\n", filename);
  }
  log_message(log_file, LOG_INFO, "ELF file mapped successfully : %s\n",
              filename);

  // Allocate memory for the sections and symbols fields
  elf_file[next_file]->sections = malloc(sizeof(SECTION_DATA));
  elf_file[next_file]->symbols = malloc(sizeof(SYMBOL_DATA));

  if (!elf_file[next_file]->sections || !elf_file[next_file]->symbols) {
    log_message(log_file, LOG_ERROR,
                "Failed to allocate memory for section and symbol data\n");
    free(elf_file[next_file]);
    free(file_data[next_file]);
    return NULL;
  }
  // Get the section header from the ELF header
  elf_file[next_file]->sections->section_header =
      (Elf32_Shdr *)((char *)elf_file[next_file]->header +
                     elf_file[next_file]->header->e_shoff);

  // Initialize the section and symbol data for the ELF file
  elf_file[next_file]->sections->section_map =
      g_hash_table_new(g_str_hash, g_str_equal);
  elf_file[next_file]->symbols->symbol_map =
      g_hash_table_new(g_str_hash, g_str_equal);
  init_section_data(elf_file[next_file]);
  init_symbol_data(elf_file[next_file]);
  file_data[next_file]->elf_file = elf_file[next_file];

  next_file++;
  return elf_file[next_file - 1];
}

// --------------------------------- Section/Symbol Data Initialization
// ---------------------------------

//@brief : Initialize the section data for the ELF file by going through each
// section and adding it to the section map with the section name as the key
// @param elf_file : The ELF file data structure
// @return : void
void init_section_data(ELF_FILE *elf_file) {
  Elf32_Ehdr *header = elf_file->header;
  Elf32_Shdr *section_header = elf_file->sections->section_header;
  Elf32_Shdr *section_string_table = &section_header[header->e_shstrndx];
  char *section_string_table_data =
      (char *)header + section_string_table->sh_offset;

  // Go through each section and add it to the section map with the section name
  // as the key
  for (int i = 0; i < header->e_shnum; i++) {
    char *section_name = &section_string_table_data[section_header[i].sh_name];
    g_hash_table_insert(elf_file->sections->section_map, section_name,
                        &section_header[i]);
  }

  if (debug) {
    printf("Section data initialized successfully\n");
  }
  log_message(log_file, LOG_INFO, "Section data initialized successfully\n");
}

//@brief : Initialize the symbol data for the ELF file by going through each
// symbol and adding it to the symbol map with the symbol name as the key and
// the symbol data as the value
//  @param elf_file : The ELF file data structure
//  @return : void
void init_symbol_data(ELF_FILE *elf_file) {
  Elf32_Ehdr *header = elf_file->header;
  Elf32_Shdr *symbol_table = NULL;
  Elf32_Shdr *string_table = NULL;

  // Find the symbol table and string table sections using the section hash map
  symbol_table =
      g_hash_table_lookup(elf_file->sections->section_map, ".symtab");
  string_table =
      g_hash_table_lookup(elf_file->sections->section_map, ".strtab");

  if (!symbol_table || !string_table) {
    printf("Failed to find symbol table or string table\n");
    log_message(log_file, LOG_ERROR,
                "Failed to find symbol table or string "
                "table for ELF file %s\n",
                file_data[current_file]->file_name);
    return;
  }
  Elf32_Sym *symbol = (Elf32_Sym *)((char *)header + symbol_table->sh_offset);
  char *string_table_data = (char *)header + string_table->sh_offset;
  int symbol_count = symbol_table->sh_size / symbol_table->sh_entsize;

  elf_file->symbols->symbol_count = symbol_count;
  elf_file->symbols->symbol_table = symbol;

  // go through each symbol and add it to the symbol map with the symbol name as
  // the key and the symbol data as the value
  for (int i = 0; i < symbol_count; i++) {
    char *symbol_name = string_table_data + symbol[i].st_name;
    g_hash_table_insert(elf_file->symbols->symbol_map, symbol_name, &symbol[i]);
  }

  if (debug) {
    printf("Symbol data initialized successfully\n");
  }
  log_message(log_file, LOG_INFO, "Symbol data initialized successfully\n");
}

// --------------------------------- ELF File Functions
// ---------------------------------
//@brief : Print the section data for the ELF file
//  @param : void
//  @return : void
void elf_file_print_sections() {
  if (next_file < 1) {
    printf("No files are opened\n");
    return;
  }
  if (next_file > 1) {
    printf("Choose a file to print sections\n");
    scanf("%d", &current_file);
    if (current_file < 0 || current_file > 1) {
      printf("Invalid file number\n");
      return;
    }
  }

  ELF_FILE *elf_file = file_data[current_file]->elf_file;
  // Create an iterator to go through the section map
  GHashTableIter iter;
  gpointer key, value;
  int i = 0;
  g_hash_table_iter_init(&iter, elf_file->sections->section_map);
  // Print the section data for the ELF file
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    char *section_name = (char *)key;
    Elf32_Shdr *curr_section = (Elf32_Shdr *)value;

    printf("[%-3d] %-16s  %08x %06x %-16hhx %-16d \n", i, section_name,
           curr_section->sh_addr, curr_section->sh_offset,
           curr_section->sh_size, curr_section->sh_type);
    i++;
  }
}

//@brief : Print the symbol data for the ELF file by going through each symbol
//  and printing the symbol data and section Name
//  @param : void
//  @return : void
void elf_file_print_symbols() {
  if (next_file < 1) {
    printf("No files are opened\n");
    return;
  }
  if (next_file > 1) {
    printf("Choose a file to print sections\n");
    scanf("%d", &current_file);
    if (current_file < 0 || current_file > 1) {
      printf("Invalid file number\n");
      return;
    }
  }
  ELF_FILE *elf_file = file_data[current_file]->elf_file;
  // Create an iterator to go through the symbol map
  GHashTableIter iter;
  gpointer key, value;
  int i = 0;
  g_hash_table_iter_init(&iter, elf_file->symbols->symbol_map);

  printf("Elf file %s:\n", file_data[current_file]->file_name);
  printf("%-20s %-10s %-10s %-16s %s\n", "Num", "Value", "Size", "section Name",
         "Symbol Name");

  Elf32_Ehdr *elf_header = elf_file->header;
  Elf32_Shdr *sections_header =
      (Elf32_Shdr *)((char *)elf_header + elf_header->e_shoff);

  // Go through each symbol and print the symbol data and section name
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    char *symbol_name = (char *)key;
    Elf32_Sym *curr_symbol = (Elf32_Sym *)value;
    char *section_name = "N/A";

    // If the symbol has a section index, get the section name
    if (curr_symbol->st_shndx != SHN_UNDEF &&
        curr_symbol->st_shndx < elf_header->e_shnum) {
      Elf32_Shdr *curr_section = &sections_header[curr_symbol->st_shndx];
      // Get the section name from the section string table using the section
      // name offset
      section_name = (char *)elf_header +
                     sections_header[elf_header->e_shstrndx].sh_offset +
                     curr_section->sh_name;
    }
    printf("[%d] %16s %08x %4s %-16x %-24s %s\n", i, "", curr_symbol->st_value,
           "", curr_symbol->st_size, section_name, symbol_name);
    i++;
  }
}

//@brief : Print the ELF header data for the ELF file
// @param : void
// @return : void
void elf_file_print_header() {
  Elf32_Ehdr *elf_header = file_data[current_file]->elf_file->header;
  unsigned char data_encoding = elf_header->e_ident[EI_DATA];
  printf("ELF Header\n");
  printf(" Magic Number: %c%c%c\n", elf_header->e_ident[EI_MAG1],
         elf_header->e_ident[EI_MAG2], elf_header->e_ident[EI_MAG3]);
  printf(" Data Encoding: %c\n", (data_encoding == ELFDATA2LSB) ? 'L' : 'B');
  printf(" Entry Point: 0x%08x\n", elf_header->e_entry);
  printf(" Section Header Offset: %u\n", elf_header->e_shoff);
  printf(" Number of Section Header Entries: %u\n", elf_header->e_shnum);
  printf(" Size of Each Section Header Entry: %u\n", elf_header->e_shentsize);
  printf(" Program Header Offset: %u\n", elf_header->e_phoff);
  printf(" Number of Program Header Entries: %u\n", elf_header->e_phnum);
  printf(" Size of Each Program Header Entry: %u\n", elf_header->e_phentsize);
}

// -----------------------------------------------------------------------------------

//@brief : Check if the symbol exists in the symbol map of FILE_2 and update the
// section index
//  @param merged_symbol_table : The merged symbol symbol_table
//  @return : void

void checkSymbols(Elf32_Sym *merged_symbol_table) {
  // Validate both file are open
  if (!file_data[FILE_1] || !file_data[FILE_2]) {
    printf("Less than 2 files are opened \n");
    return;
  }
  // Get the string table of FILE_1
  Elf32_Shdr *string_table_section =
      g_hash_table_lookup(elf_file[FILE_1]->sections->section_map, ".strtab");
  // Get the string table data of FILE_1
  char *string_table =
      (char *)elf_file[FILE_1]->header + string_table_section->sh_offset;
  // Get the symbol count of FILE_1
  int symbol_count = elf_file[FILE_1]->symbols->symbol_count;

  // Iterate through the symbol table of file_name_1
  for (int i = 1; i < symbol_count; i++) {
    Elf32_Sym *symbol = &merged_symbol_table[i];
    char *symbol_name = (char *)string_table + symbol->st_name;

    // Skip the dummy symbol
    if (symbol->st_name == 0 || symbol_name[0] == '\0') {
      continue;
    }

    // Search for the symbol in the symbol map of FILE_2
    Elf32_Sym *symbol2 =
        g_hash_table_lookup(elf_file[FILE_2]->symbols->symbol_map, symbol_name);
    if (symbol2) {
      if (symbol->st_shndx == SHN_UNDEF && symbol2->st_shndx != SHN_UNDEF) {
        // deep copy the symbol2 to symbol
        symbol->st_shndx = symbol2->st_shndx;
        symbol->st_value = symbol2->st_value;
        log_message(log_file, LOG_INFO,
                    "Symbol %s found in FILE_2 and updated section index\n",
                    symbol_name);
      }
    }
  }
  log_message(log_file, LOG_INFO, "Symbol table checked successfully\n");
}

// --------------------------------- Merge ELF Files
// ---------------------------------
//@brief : Merge the ELF files by writing the ELF header, section headers,
// section data, and symbol table to the output file out.ro
// @param : void
// @return : void

// WARNING: The merged_section_offset cause probelms if not updated correctly
void merge_elf_files() {

  // Validate both file are open
  if (!file_data[FILE_1] || !file_data[FILE_2]) {
    printf("Less than 2 files are opened \n");
    return;
  }

  int out_fd = open("F12a.o", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (out_fd == -1) {
    log_message(log_file, LOG_ERROR, "Failed to open the output file\n");
    perror("Failed to open the output file");
    exit(1);
  }

  // Write FILE_1 to the output file
  write(out_fd, elf_file[FILE_1]->header, sizeof(Elf32_Ehdr));

  // Allocate memory for the merged section headers
  Elf32_Shdr *merged_section_headers =
      malloc(elf_file[FILE_1]->header->e_shnum * sizeof(Elf32_Shdr));

  // Copy the section headers of FILE_1 to the merged section
  // merged_section_headers
  memcpy(merged_section_headers, elf_file[FILE_1]->sections->section_header,
         elf_file[FILE_1]->header->e_shnum * sizeof(Elf32_Shdr));

  // Keep track of the offset of the merged section
  off_t merged_section_offset = lseek(out_fd, 0, SEEK_END);

  // Iterate through the section headers of FILE_1
  for (int i = 0; i < elf_file[FILE_1]->header->e_shnum; i++) {
    // Get the section of the merged section headers
    Elf32_Shdr *section_header = &merged_section_headers[i];
    // Get the section data of FILE_1
    char *section_data =
        (char *)elf_file[FILE_1]->header + section_header->sh_offset;

    char *section_name =
        (char *)elf_file[FILE_1]->header +
        file_data[FILE_1]
            ->elf_file->sections
            ->section_header[elf_file[FILE_1]->header->e_shstrndx]
            .sh_offset +
        section_header->sh_name;

    Elf32_Shdr *section2 = NULL;
    if (strcmp(section_name, ".text") == 0) {
      section2 =
          g_hash_table_lookup(elf_file[FILE_2]->sections->section_map, ".text");
    }
    if (strcmp(section_name, ".rodata") == 0) {
      section2 = g_hash_table_lookup(elf_file[FILE_2]->sections->section_map,
                                     ".rodata");
    }
    if (strcmp(section_name, ".data") == 0) {
      section2 =
          g_hash_table_lookup(elf_file[FILE_2]->sections->section_map, ".data");
    }
    if (strcmp(section_name, ".symtab") == 0) {
      // Get the symbol table of FILE_1
      Elf32_Sym *original_symbol_table =
          (Elf32_Sym *)((char *)elf_file[FILE_1]->header +
                        section_header->sh_offset);

      // Allocate memory for the merged symbol table
      Elf32_Sym *merged_symbol_table = malloc(section_header->sh_size);

      // Copy the symbol table of FILE_1 to the merged symbol table
      memcpy(merged_symbol_table, original_symbol_table,
             section_header->sh_size);

      // Create the merged symbol table
      checkSymbols(merged_symbol_table);

      // Write the merged symbol table to the output file
      write(out_fd, merged_symbol_table, section_header->sh_size);

      // Update the section header offset
      section_header->sh_offset = merged_section_offset;
      // Update the merged section offset
      merged_section_offset = lseek(out_fd, 0, SEEK_END);
      free(merged_symbol_table);
      log_message(log_file, LOG_INFO, "Symbol table merged successfully\n");
      continue;
    }

    // .TEXT, .RODATA, .DATA sections in FILE_1 and FILE_2
    if (section2) {
      // Write the section data of FILE_1 to the output file
      write(out_fd, section_data, section_header->sh_size);

      // Write the section data of FILE_2 to the output file
      write(out_fd, (char *)elf_file[FILE_2]->header + section2->sh_offset,
            section2->sh_size);

      // Update the section header offset and size
      section_header->sh_offset =
          merged_section_offset; // How far into the file

      section_header->sh_size += section2->sh_size; // How big the section is

      // Update the merged section Offset
      merged_section_offset = lseek(
          out_fd, 0, SEEK_END); // How far into the file with the new section
      log_message(log_file, LOG_INFO, "Section %s merged successfully\n",
                  section_name);
      continue;
    }

    // Write the section data of FILE_1 to the output file
    write(out_fd, section_data, section_header->sh_size);
    // Update the section header offset
    section_header->sh_offset = merged_section_offset;
    // Update the merged section offset
    merged_section_offset = lseek(out_fd, 0, SEEK_END);
    log_message(log_file, LOG_INFO, "Section %s successfully written\n",
                section_name);
  }

  // Write the merged section headers to the output file
  lseek(out_fd, merged_section_offset, SEEK_SET);
  write(out_fd, merged_section_headers,
        elf_file[FILE_1]->header->e_shnum * sizeof(Elf32_Shdr));

  // Copy FILE_1 ELF header to the output file
  Elf32_Ehdr *elf_header = malloc(sizeof(Elf32_Ehdr));
  memcpy(elf_header, elf_file[FILE_1]->header, sizeof(Elf32_Ehdr));

  // Fix the e_shoff field in the ELF headers
  elf_header->e_shoff = merged_section_offset;

  // Write the ELF header to the output file
  lseek(out_fd, 0, SEEK_SET);
  write(out_fd, elf_header, sizeof(Elf32_Ehdr));

  // Close the output file
  close(out_fd);

  // Free the allocated memory
  free(merged_section_headers);
  free(elf_header);
  log_message(log_file, LOG_INFO, "Files merged successfully\n");
}

void elf_file_destroy() {
  for (int i = 0; i < next_file; i++) {
    ELF_FILE *elf = elf_file[i];
    munmap(elf->header, file_data[i]->file_size);
    close(file_data[i]->fd);
    free(elf->sections);
    free(elf->symbols);
    free(elf);
    free(file_data[i]);
  }
  log_message(log_file, LOG_INFO, "Files closed successfully\n");
  exit(0);
}

void printMenu(struct menu_desc menu[], int menu_size) {
  printf("Choose an action:\n");
  for (int i = 0; i < menu_size; i++) { // Print all menu options
    printf("%d) %s \n", i, menu[i].action);
  }
}

// --------------------------------- Main Function
// ---------------------------------
int main(int argc, char **argv) {
  log_file = fopen("logfile.txt", "a"); // Open log file for appending

  if (log_file == NULL) {
    fprintf(stderr, "Error opening log file\n");
    return 1;
  }
  struct menu_desc menu[] = {{"Toggle Debug Mode", toggleDebug},
                             {"Load ELF File", elf_load_file},
                             {"Examine ELF File", elf_file_print_header},
                             {"Print Section Names", elf_file_print_sections},
                             {"Print Symbols", elf_file_print_symbols},
                             {"Merge ELF Files", merge_elf_files},
                             {"Quit", elf_file_destroy}};

  int menu_size = sizeof(menu) / sizeof(menu[0]);
  while (1) {
    printMenu(menu, menu_size);

    int chosen_func;
    if (scanf("%d", &chosen_func) != 1) {
      printf("Error: Invalid input \n");
      __fpurge(stdin); // Clean the buffer
      continue;
    }

    if (chosen_func >= 0 && chosen_func < menu_size) {
      (menu[chosen_func].func)();
      log_message(log_file, LOG_INFO, "Action %s executed successfully\n",
                  menu[chosen_func].action);
    } else {
      printf("Error: Value %d is out of bounds\n", chosen_func);
    }
  }
}
