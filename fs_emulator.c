#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define I_NODES 1024
#define FILE_NAME_POS 1
#define NUM_CHARS 32
#define BUFF_SIZE 8192

void validate_params(int argc, char *argv[])
{
    if (argc < 2 || argv[FILE_NAME_POS] == NULL)
    {
        perror("Validate params");
        exit(1);
    }
}

void* checked_malloc(size_t size) {
    void *p;
    p = malloc(size);
    if (p == NULL) {
        perror("malloc");
        exit(1);
    }
    return p;
}

char *uint32_to_str(uint32_t i)
{
   int length = snprintf(NULL, 0, "%lu", (unsigned long)i);       // pretend to print to a string to get length
   char* str = checked_malloc(length + 1);                        // allocate space for the actual string
   snprintf(str, length + 1, "%lu", (unsigned long)i);            // print to string

   return str;
}

// must be in the root directory where "inodes_list" exists
void open_inodes_list(char inodes[I_NODES], uint32_t *size)
{
    FILE *file = fopen("inodes_list", "rb");
    while (*size < I_NODES)
    {
        if (file == NULL)
        {
            perror("File not found");
            return;
        }
        uint32_t inode;
        char type;
        while(fread(&inode, sizeof(uint32_t), 1, file)) {
            int n = fread(&type, sizeof(char), 1, file);
            inodes[inode] = type;
            ++(*size);
        }
    }
}

void read_directory(uint32_t root)
{
    FILE *file = fopen(uint32_to_str(root), "rb");
    if (file == NULL) {
        perror("Read directory");
        return;
    }
    uint32_t inode;
    char type;
    while(fread(&inode, sizeof(uint32_t), 1, file)) {
        int n = fread(&type, sizeof(char), 1, file);
        FILE* inode_file = fopen(uint32_to_str(inode), "rb");
        char* filename = NULL;
        fread(filename, sizeof(char), 32, inode_file);
        printf("%u %s\n", inode, filename);
        fclose(inode_file);
    }

    fclose(file);
}


void cd(char* dir, uint32_t* curr_dir) {
    FILE *file = fopen(uint32_to_str(*curr_dir), "rb");
    uint32_t inode;
    char type;
    while(fread(&inode, sizeof(uint32_t), 1, file)) {
        int n = fread(&type, sizeof(char), 1, file);
        FILE* inode_file = fopen(uint32_to_str(inode), "rb");
        char* filename = NULL;
        fread(filename, sizeof(char), 32, inode_file);
        if (strcmp(filename, dir) == 0) {
            *curr_dir = inode;
        }
        fclose(inode_file);
    }
}

void ls(uint32_t inode) {
    
}

void mkdir(char* dir) {
    
}

void touch(char* filename) {
    
}


int main(int argc, char *argv[])
{
    validate_params(argc, argv);
    char *filename = argv[FILE_NAME_POS];
    char inodes[I_NODES];
    uint32_t curr_dir = 0;

    // open the directory specified in argv
    int fd = open(filename, O_DIRECTORY);
    if (fd == -1)
    {
        perror("Check directory");
        exit(1);
    }

    // directory located
    int success = chdir(filename);
    if (success != 0)
    {
        perror("Changing directory");
        exit(1);
    }

    uint32_t size = 0;
    open_inodes_list(0, &size);

    char* line;
    size_t length;

    read_directory(curr_dir);
    
    while (getline(&line, &length, stdin) > 0) {
        char* orig_line = line;
        char *token = NULL;
        bool cd_ = false, ls_ = false, mkdir_ = false, touch_ = false;
        while ((token = strsep(&orig_line, " \n\t\r  ")) != NULL)
        {
            if (strcmp(token, "^D") == 0) 
                exit(0);
            else if (strcmp(token, "cd") == 0) 
                cd_ = true;
            else if (strcmp(token, "ls") == 0) 
                ls_ = true;
            else if (strcmp(token, "mkdir") == 0) 
                mkdir_ = true; 
            else if (strcmp(token, "touch") == 0) 
                touch_ = true;
            else if (cd_) {
                cd(token, &curr_dir);
            }
            else if (ls_) 
                ls(curr_dir);
            else if (mkdir_) 
                mkdir(token);
            else if (touch_) 
                touch(token);
            else {printf("\"%s\" is not recognized as a command\n", token); break;}
        }
        free(line);
    }

}