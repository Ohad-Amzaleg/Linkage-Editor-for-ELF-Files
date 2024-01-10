#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <elf.h>
#define FILE_NUM 2 //Currently we defiend 2 files to be given
#define FILE_1 0
#define FILE_2 1 



typedef union {
    void (*func);
} func_ptr;


struct fun_desc {
char *name;
func_ptr func;
};



// ############# global vars ##########
int debug_mode;
int fd_arr[FILE_NUM];
void* mapStart_arr[FILE_NUM];
char* fileName_arr[FILE_NUM];
off_t file_size_arr[FILE_NUM];


//############ Program Functions########
void toggleDebugMode();
void examineELF();
void printSection();
void printSymbols();
void checkFiles(Elf32_Sym* symbol_table_copy);
void checkSymbols(int num_symbols[],Elf32_Sym* symbol_table[], char *string_table[],bool modify);
void mergeELF();
void quit();





//#################### main program ############33
int main(int argc, char **argv){
fd_arr[0]=-1;
fd_arr[1]=-1;
mapStart_arr[0]=MAP_FAILED;
mapStart_arr[1]=MAP_FAILED;
char filename[100];



//menu array with all functions
struct fun_desc menu[] = {  
  {"Toggle Debug Mode", {.func=toggleDebugMode}},
  {"Examine ELF File",{.func=examineELF}},
  {"Print Section Names",{.func=printSection}},
  {"Print Symbols",{.func=printSymbols}},
  {"Check Files for Merge",{.func=checkFiles}},
  {"Merge ELF Files ",{.func=mergeELF}},
  {"Quit",{.func=quit}},
  {NULL,{.func=NULL}}
};

int menuSize=sizeof(menu)/sizeof(menu[0]);
while(1){
  printf("please choose a function:\n");

  //Show all available functions in menu
  for(int i=0;i<menuSize-1;i++){
    printf("%d) %s \n",i,menu[i].name);
  }

 if (fgets(filename, 100, stdin) == NULL) {
            continue;
    }

  int chosenFunction;
    if (sscanf(filename, "%d", &chosenFunction) != 1) {
        printf("Invalid filename\n");
        continue;
    }
 
  //check if filename is within bounds
  if(chosenFunction>=0 && chosenFunction<menuSize){
    printf("Within bounds\n");
    switch (chosenFunction) {
        case 0: toggleDebugMode();
        break;
        
        case 1:examineELF();
        break; 

        case 2:printSection();
        break;

        case 3:printSymbols();
        break;

        case 4:checkFiles(NULL);
        break;

        case 5:mergeELF();
        break;

        case 6:quit();
        break; 
    }

  }

  //if filename is not within bounds exit
  else{
    printf("Not within bounds\n");
    break;
  } 

}

}


void toggleDebugMode() {
  if (debug_mode == 0) {
    debug_mode = 1;
    fprintf(stderr, "Debug flag now on\n");
  } else {
    debug_mode = 0;
    fprintf(stderr, "Debug flag now off\n");
  }
}


void examineELF() {
  int index=0;
  
  if(file_size_arr[0]>0){
    index=1;
  }

    char filename[100];

 if (fgets(filename, 100, stdin) == NULL) {
        return;
    }
    filename[strcspn(filename, "\n")] = '\0';
    
    fd_arr[index] = open(filename, O_RDONLY);
    if (fd_arr[index] == -1) {
        perror("Error opening file");
        return;
    }
     fileName_arr[index] = strdup(filename);
    file_size_arr[index] = lseek(fd_arr[index], 0, SEEK_END);
    mapStart_arr[index] = mmap(NULL, file_size_arr[index], PROT_READ, MAP_SHARED, fd_arr[index], 0);
    if (mapStart_arr[index] == MAP_FAILED) {
        perror("Error mapping file");
        close(fd_arr[index]);
        fd_arr[index] = -1;
        return;
    }
    
    Elf32_Ehdr *elf_header = (Elf32_Ehdr *)mapStart_arr[index];
    if (elf_header->e_ident[EI_MAG0] != ELFMAG0 || elf_header->e_ident[EI_MAG1] != ELFMAG1 ||
        elf_header->e_ident[EI_MAG2] != ELFMAG2 || elf_header->e_ident[EI_MAG3] != ELFMAG3) {
        printf("Not an ELF file\n");
        munmap(mapStart_arr[index], file_size_arr[index]);
        close(fd_arr[index]);
        fd_arr[index] = -1;
        return;
    }
   
    unsigned char data_encoding = elf_header->e_ident[EI_DATA];
    printf("ELF Header\n");
    printf(" Magic Number: %c%c%c\n", elf_header->e_ident[EI_MAG1], elf_header->e_ident[EI_MAG2], elf_header->e_ident[EI_MAG3]);
    printf(" Data Encoding: %c\n", (data_encoding == ELFDATA2LSB) ? 'L' : 'B');
    printf(" Entry Point: 0x%08x\n", elf_header->e_entry);
    printf(" Section Header Offset: %u\n",  elf_header->e_shoff);
    printf(" Number of Section Header Entries: %u\n", elf_header->e_shnum);
    printf(" Size of Each Section Header Entry: %u\n", elf_header->e_shentsize);
    printf(" Program Header Offset: %u\n", elf_header->e_phoff);
    printf(" Number of Program Header Entries: %u\n", elf_header->e_phnum);
    printf(" Size of Each Program Header Entry: %u\n", elf_header->e_phentsize);


    
}


void printSection(){
for(int index=0;index<FILE_NUM;index++){
    if(file_size_arr[index]== 0)
        return;
        

    Elf32_Ehdr *elf_header = (Elf32_Ehdr *)mapStart_arr[index];
    //First convert elf header to pointer to do arithmetics 
    //Then appaned the section headers offset within the elf 
    Elf32_Shdr *section_headers=(Elf32_Shdr *)((char*)(elf_header) + elf_header->e_shoff);
    
    printf("File Elf %s:\n",fileName_arr[index]);
    printf("%-20s %-10s %-10s %-10s %-10s\n", "Name", "Addr", "Offset", "Size","Type");
    
    for(int i=0;i<elf_header->e_shnum;i++){
    Elf32_Shdr *curr_section = &section_headers[i];
    //Elf start point + offset to the names table + offset within the names table
    char *section_name = (char *)elf_header + section_headers[elf_header->e_shstrndx].sh_offset + curr_section->sh_name;

    printf("[%-3d] %-16s  %08x %06x %-16hhx %-16d \n", 
            i,section_name, curr_section->sh_addr, curr_section->sh_offset, curr_section->sh_size,curr_section->sh_type);
    
      if(debug_mode){
      printf("Debug Mode Data \n");
      printf("Section offset : %u\n",curr_section->sh_offset);
      printf("Section string table index :%u",elf_header->e_shstrndx);
      }
    }

}

}

void printSymbols(){
for(int index=0;index<FILE_NUM;index++){
    if(file_size_arr[index]== 0)
        return;

    Elf32_Ehdr *elf_header = (Elf32_Ehdr *)mapStart_arr[index];
    //First convert elf header to pointer to do arithmetics 
    //Then appaned the section headers offset within the elf 
    Elf32_Shdr *section_headers=(Elf32_Shdr *)((char*)(elf_header) + elf_header->e_shoff);
    
    printf("Elf file %s:\n",fileName_arr[index]);
    printf("%-20s %-10s %-10s %-16s %s\n", "Num", "Value", "Size", "section Name","Symbol Name");
    Elf32_Shdr* symbol_table_section;
    Elf32_Shdr *string_table_section;

    //Finding the symbols section header 
    for(int i=0;i<elf_header->e_shnum;i++){
        Elf32_Shdr *curr_section = &section_headers[i];
        char *section_name = (char *)elf_header + section_headers[elf_header->e_shstrndx].sh_offset + curr_section->sh_name;
        // printf("name : %s",section_name);

        if(strcmp(section_name,".symtab") == 0){
            symbol_table_section=&section_headers[i];
            string_table_section = &section_headers[section_headers[i].sh_link];

        }
    }

    // //Finding the actual table using the offset in the header table
    Elf32_Sym* symbol_table=(Elf32_Sym*) ( (char*)(elf_header) + symbol_table_section->sh_offset);
    char *string_table = (char *)(elf_header) +  string_table_section->sh_offset;

    // //Get the number of symbols in the symbol table using the header size divided by the entries size 
    int num_symbols = symbol_table_section->sh_size / symbol_table_section->sh_entsize;

    // //Iterate over all of the symbols inside the string table 
    for (int i = 0; i < num_symbols; ++i) {
        Elf32_Sym* curr_symbol = &symbol_table[i];       
        char* symbol_name= string_table + curr_symbol->st_name;        
        char* section_name = "N/A";

        if (curr_symbol->st_shndx != SHN_UNDEF && curr_symbol->st_shndx < elf_header->e_shnum) {
          Elf32_Shdr* curr_section = &section_headers[curr_symbol->st_shndx];
          section_name = (char *)elf_header + section_headers[elf_header->e_shstrndx].sh_offset + curr_section->sh_name;
        }

        printf("[%d] %16s %08x %4s %-16x %-24s %s\n", i,"",curr_symbol->st_value,"",curr_symbol->st_size,section_name, symbol_name);

    
    }


}





}





void checkFiles(Elf32_Sym* symbol_table_copy){
  if( file_size_arr[FILE_1]==0 || file_size_arr[FILE_2]==0){
    perror("There is less than FILE_NUM files opened!");
    return;
  }

    Elf32_Ehdr *elf_header [FILE_NUM];
    Elf32_Shdr *section_headers[FILE_NUM];
    Elf32_Shdr* symbol_table_section[FILE_NUM];
    Elf32_Shdr *string_table_section[FILE_NUM];
    Elf32_Sym* symbol_table[FILE_NUM];
    char *string_table[FILE_NUM];
    int num_symbols[FILE_NUM];

    for (int index=0; index<FILE_NUM; index++) {

      elf_header[index]=(Elf32_Ehdr *)mapStart_arr[index];
      
      section_headers[index]=(Elf32_Shdr *)((char*)(elf_header[index]) + elf_header[index]->e_shoff);

        for(int i=0;i<elf_header[index]->e_shnum;i++){
        Elf32_Shdr *curr_section = &section_headers[index][i];

        char *section_name = (char *)(elf_header[index]) + section_headers[index][elf_header[index]->e_shstrndx].sh_offset + curr_section->sh_name;

        if(strcmp(section_name,".symtab") == 0){
            symbol_table_section[index]=&section_headers[index][i];
            string_table_section[index]= &section_headers[index][section_headers[index][i].sh_link];

        }
    }
    
      symbol_table[index]=(Elf32_Sym*) ( (char*)(elf_header[index]) + symbol_table_section[index]->sh_offset);
      string_table[index] = (char *)(elf_header[index]) +  string_table_section[index]->sh_offset;
      num_symbols[index] = symbol_table_section[index]->sh_size / symbol_table_section[index]->sh_entsize;

    }

      if(symbol_table_copy!=NULL){
        symbol_table[FILE_1]=symbol_table_copy;
        checkSymbols(num_symbols,symbol_table,string_table,true);

      }

      else{
       checkSymbols(num_symbols,symbol_table,string_table,false);
      }
}





void checkSymbols(int num_symbols[],Elf32_Sym* symbol_table[], char *string_table[],bool modify){


   // Loop over symbols in symbol table 1
    for (int i = 1; i < num_symbols[FILE_1]; ++i) {
        Elf32_Sym* symbol1 = &symbol_table[FILE_1][i];

        // Skip dummy null symbol
        if (symbol1->st_name == 0)
            continue;

        const char* symbol_name1 = (const char*)string_table[FILE_1] + symbol1->st_name; 

        // Check if symbol1 is UNDEFINED
        if (symbol1->st_shndx == SHN_UNDEF) {
            // Search for symbol1 in symbol table 2

            int found = 0;

            for (int j = 1; j < num_symbols[FILE_2]; ++j) {

                Elf32_Sym* symbol2 = &symbol_table[FILE_2][j];
                const char* symbol_name2 = (const char*)string_table[FILE_2] + symbol2->st_name; 

                if (strcmp(symbol_name1, symbol_name2) == 0 && symbol2->st_shndx != SHN_UNDEF) {
                    found = 1;
                    if(modify){
                      symbol1->st_shndx=symbol2->st_shndx;
                    }
                    break;
                }
            }

            if (!found) {
                printf("Symbol '%s' undefined.\n", symbol_name1);
            }

        } 
        
        else {
            // Check if symbol1 is defined (has a valid section number)
            if (symbol1->st_shndx != SHN_UNDEF) {
                // Search for symbol1 in symbol table 2
                for (int j = 1; j < num_symbols[FILE_2]; ++j) {

                    Elf32_Sym* symbol2 = &symbol_table[FILE_2][j];
                
                    const char* symbol_name2 = (const char*)string_table[FILE_2] + symbol2->st_name; 
                
                    if (strcmp(symbol_name1, symbol_name2) == 0 && symbol2->st_shndx != SHN_UNDEF) {
                        printf("Symbol '%s' multiply defined.\n", symbol_name1);
                        break;
                    }
                
                }
            }
        }
    }
}








void mergeELF(){
    checkFiles(NULL);
    
    // Open the output file "out.ro"
    int out_fd = open("out.ro", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd == -1) {
        perror("Failed to open the output file");
        exit(1);
    }


    Elf32_Ehdr *elf_header [FILE_NUM];
    Elf32_Shdr *section_headers[FILE_NUM];

    for (int index = 0; index < FILE_NUM; index++) {
       elf_header[index]=(Elf32_Ehdr *)mapStart_arr[index];
      section_headers[index]=(Elf32_Shdr *)((char*)(elf_header[index]) + elf_header[index]->e_shoff);
  }


    // Copy the ELF header of the first file as the header of the merged file
    write(out_fd, elf_header[FILE_1], sizeof(Elf32_Ehdr));

    // Create an initial version of the section header table for the merged file
    Elf32_Shdr* merged_sections = malloc(elf_header[FILE_1]->e_shnum * sizeof(Elf32_Shdr));

    memcpy(merged_sections, section_headers[FILE_1], elf_header[FILE_1]->e_shnum * sizeof(Elf32_Shdr));


    // Find the ".text" section in the section header table of the second file
    Elf32_Shdr* file2_text_section = NULL;
    Elf32_Shdr* file2_data_section = NULL;
    Elf32_Shdr* file2_rodata_section = NULL;



    for (int i = 0; i < elf_header[FILE_2]->e_shnum; i++) {
        Elf32_Shdr* current_section=&section_headers[FILE_2][i];
        char* section_name = (char*)elf_header[FILE_2] + section_headers[FILE_2][elf_header[FILE_2]->e_shstrndx].sh_offset +  current_section->sh_name;
        
        if (strcmp(section_name, ".text") == 0) {
            file2_text_section = &section_headers[FILE_2][i];
        }
         if (strcmp(section_name, ".data") == 0) {
            file2_data_section = &section_headers[FILE_2][i];
        }
         if (strcmp(section_name, ".rodata") == 0) {
            file2_rodata_section = &section_headers[FILE_2][i];
        }
    
    }


    // Offset to keep track of the merged sections in the output file
    off_t merged_sections_offset = lseek(out_fd, 0, SEEK_END);

    // Loop over the entries of the new section header table and process each section
    for (int i = 0; i < elf_header[FILE_1]->e_shnum; i++) {
        Elf32_Shdr*  current_section = &merged_sections[i];
        
        char* section_name = (char*)(elf_header[FILE_1]) + section_headers[FILE_1][elf_header[FILE_1]->e_shstrndx].sh_offset +  current_section->sh_name;
        

        if (strcmp(section_name, ".text") == 0) {
           write(out_fd, (char*)elf_header[FILE_1] + current_section->sh_offset, current_section->sh_size);
        
            if (file2_text_section != NULL) {
            write(out_fd, (char*)elf_header[FILE_2] + file2_text_section->sh_offset, file2_text_section->sh_size);
           
            //Update the current section new size and offset 
            current_section->sh_offset= merged_sections_offset;
            //Update the current offset from the file start point 
            merged_sections_offset=lseek(out_fd, 0, SEEK_CUR);

            current_section->sh_size += file2_text_section->sh_size;
           
            }

             continue;
        }


        if (strcmp(section_name, ".data") == 0) {
           write(out_fd, (char*)elf_header[FILE_1] + current_section->sh_offset, current_section->sh_size);

            if (file2_data_section != NULL) {
            write(out_fd, (char*)elf_header[FILE_2] + file2_data_section->sh_offset, file2_data_section->sh_size);
            
            //Update the current section new size and offset 
            current_section->sh_offset= merged_sections_offset;

            //Update the current offset from the file start point 
            merged_sections_offset=lseek(out_fd, 0, SEEK_CUR);

            current_section->sh_size += file2_data_section->sh_size;

            
            }

          continue;

        }


        if (strcmp(section_name, ".rodata") == 0) {
           write(out_fd, (char*)elf_header[FILE_1] + current_section->sh_offset, current_section->sh_size);
        
            if (file2_rodata_section != NULL) {
            write(out_fd, (char*)elf_header[FILE_2] + file2_rodata_section->sh_offset, file2_rodata_section->sh_size);

            //Update the current section new size and offset 
            current_section->sh_offset= merged_sections_offset; 
            //Update the current offset from the file start point 
            merged_sections_offset=lseek(out_fd, 0, SEEK_CUR);

            current_section->sh_size += file2_rodata_section->sh_size;


            }
            continue;
        }

         if(strcmp(section_name,".symtab") == 0){
              //Original table 
              Elf32_Sym* symbol_table=(Elf32_Sym*) ( (char*)(elf_header[FILE_1]) + current_section->sh_offset);
              //Copy in memory of the table 
              Elf32_Sym* symbol_table_copy = malloc(symbol_table->st_size * sizeof(Elf32_Sym));
              memcpy(symbol_table_copy, symbol_table, symbol_table->st_size * sizeof(Elf32_Sym));

              //Modify the table 
              checkFiles(symbol_table_copy);

              //Write the new one into the file 
              write(out_fd,symbol_table_copy, current_section->sh_size);
              //Update the current section new size and offset 
              current_section->sh_offset= merged_sections_offset; 
              //Update the current offset from the file start point 
              merged_sections_offset=lseek(out_fd, 0, SEEK_CUR);
              free(symbol_table_copy);
              continue;
          }


        write(out_fd, (char*)elf_header[FILE_1] + current_section->sh_offset, current_section->sh_size);
        //Update the current section new size and offset 
        current_section->sh_offset= merged_sections_offset; 
       //Update the current offset from the file start point 
        merged_sections_offset=lseek(out_fd, 0, SEEK_CUR);
    }


    // Write the modified section header table entry for the merged ".text" section
    lseek(out_fd, merged_sections_offset, SEEK_SET);
    write(out_fd, merged_sections, elf_header[FILE_1]->e_shnum * sizeof(Elf32_Shdr));
    
    
    Elf32_Ehdr *copy_elf_header= malloc(sizeof(Elf32_Ehdr));
    memcpy(copy_elf_header,  elf_header[FILE_1], sizeof(Elf32_Ehdr));

    // Fix the "e_shoff" field in the ELF header of "out.ro"
    copy_elf_header->e_shoff = merged_sections_offset;

    // Write the updated ELF header to "out.ro"
    lseek(out_fd, 0, SEEK_SET);
    write(out_fd,copy_elf_header, sizeof(Elf32_Ehdr));

    // Close the output file
    close(out_fd);

    // Free the allocated memory
    free(merged_sections);
    free(copy_elf_header);

  }











void quit(){
    if(file_size_arr[FILE_1]>0){
    close(fd_arr[FILE_1]);
    munmap(mapStart_arr[FILE_1],file_size_arr[FILE_1]);
    free(fileName_arr[FILE_1]);
    }
    if(file_size_arr[FILE_2]>0){
    close(fd_arr[FILE_2]);
    munmap(mapStart_arr[FILE_2], file_size_arr[FILE_2]);
    free(fileName_arr[FILE_2]);
    }
    exit(0);
}

