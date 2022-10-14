#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define I_NODES 1024
#define FILE_NAME_POS 1
#define NUM_CHARS 33
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
        *size = *size + 1;
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
    if (file == NULL) perror("cd");
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
    if (file != NULL) fclose(file);
}

void ls(uint32_t inode) {
    read_directory(inode);
}

void mkdir(char* dir) {
    
}

// String supplied must be of length NUM_CHARS
void null_truncate(char* str) {
    size_t len = strlen(str);
    for (int i = len; i <= NUM_CHARS; ++i) {
        str[i] = '\0';
    }
}

void touch(char* target, uint32_t *curr_dir, uint32_t *size, char* inodes) {
    FILE *file = fopen(uint32_to_str(*curr_dir), "rb");
    if (strlen(target) > NUM_CHARS) {
        printf("Filename is too large");
        return;
    }
    if (file == NULL) perror("touch");
    // check that the file doesn't already exist
    uint32_t inode;
    char type;
    while(fread(&inode, sizeof(uint32_t), 1, file)) {
        char filename[NUM_CHARS];
        fread(filename, sizeof(char), 32, file);
        if (strcmp(filename, target) == 0) {
            printf("Error: File already exists");
            return;
        }
    }
    fclose(file);

    // file doesn't exist, we can now create it in the virtual directory
    file = fopen(uint32_to_str(*curr_dir), "ab");
    fwrite(size, sizeof(uint32_t), 1, file);
    char buffer[NUM_CHARS];
    strcpy(buffer, target);
    null_truncate(buffer);
    fwrite(buffer, sizeof(buffer), 1, file);
    inodes[*size] = 'f';
    *size = *size + 1;
    fclose(file);


    // update inodes_list to reflect that the file exists
    file = fopen("inodes_list", "ab");
    fwrite(size, sizeof(uint32_t), 1, file);
    char filetype = 'f';
    fwrite(&filetype, sizeof(char), 1, file);
    fclose(file);
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
    
    while (getline(&line, &length, stdin) > 0) {
        char *token = NULL;
        bool cd_ = false, ls_ = false, mkdir_ = false, touch_ = false;
        char* delim = " \n\t\r";
        token = strtok(line, delim);
        while (token != NULL) {
            if (strcmp(token, "^D") == 0 || strcmp(token, "exit") == 0) 
                exit(0);
            else if (strcmp(token, "cd") == 0) 
                cd_ = true;
            else if (strcmp(token, "ls") == 0) 
                ls(curr_dir);
            else if (strcmp(token, "mkdir") == 0) 
                mkdir_ = true; 
            else if (strcmp(token, "touch") == 0) 
                touch_ = true;
            else if (cd_) {
                cd(token, &curr_dir, inodes);
            }
            else if (mkdir_) 
                mkdir(token);
            else if (touch_) 
                touch(token, &curr_dir, &size, inodes);
            else {
                printf("\"%s\" is not recognized as a command\n", token); break;
            }
            token = strtok(NULL, delim);
        }
    }
    free(line);
}