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
// inodes array must have size of I_NODES
void open_inodes_list(char* inodes, uint32_t *size)
{
    FILE *file = fopen("inodes_list", "rb");
    if (file == NULL)
    {
        perror("open_inodes_list");
        return;
    }
    uint32_t inode;
    char type;
    while(fread(&inode, sizeof(uint32_t), 1, file)) {
        fread(&type, sizeof(char), 1, file);
        inodes[inode] = type;
        ++(*size);
    }
    fclose(file);
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
    // read the int value of file
    while(fread(&inode, sizeof(uint32_t), 1, file)){
        char filename[NUM_CHARS];
        fread(filename, sizeof(char), 32, file);
        printf("%u %s\n", inode, filename);
    }
    fclose(file);
}


void cd(char* dir, uint32_t* curr_dir, char* inodes) {
    FILE *file = fopen(uint32_to_str(*curr_dir), "rb");
    uint32_t inode;
    char type;
    while(fread(&inode, sizeof(uint32_t), 1, file)) {
        char filename[NUM_CHARS];
        fread(filename, sizeof(char), 32, file);
        if (strcmp(filename, dir) == 0) {
            // if trying to cd into a file, notify that's not possible
            if (inodes[inode] == 'f')
                printf("Error: Can't change directory into file\n");
            else
                *curr_dir = inode;
        }
    }
    fclose(file);
}

void ls(uint32_t inode) {
    read_directory(inode);
}

void mkdir(char* dir) {
    
}

void touch(char* filename, uint32_t *size, char* inodes) {
    ++(*size);
    inodes
    // TODO: Create file
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
    open_inodes_list(inodes, &size);

    char* line = NULL;
    size_t length;

    read_directory(curr_dir);
    
    while (getline(&line, &length, stdin) > 0) {
        char* orig_line = line;
        char *token = NULL;
        bool cd_ = false, ls_ = false, mkdir_ = false, touch_ = false;
        while ((token = strsep(&orig_line, " \n\t\r")) != NULL)
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
                cd(token, &curr_dir, inodes);
            }
            else if (ls_) 
                ls(curr_dir);
            else if (mkdir_) 
                mkdir(token);
            else if (touch_) 
                touch(token, &size, inodes);
            else {printf("\"%s\" is not recognized as a command\n", token); break;}
        }
    }
    free(line);
}