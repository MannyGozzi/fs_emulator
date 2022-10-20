#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>

#define I_NODES 1024
#define DIR_NAME_POS 1
#define NUM_CHARS 32
#define BUFF_SIZE 8192

void validate_params(int argc, char *argv[])
{
    if (argc < 2 || argv[DIR_NAME_POS] == NULL)
    {
        printf("\033[33mPlease enter a valid directory when running the program. \nE.g. <program_name> <directory_name>\033[0m\n");
        exit(1);
    }
}

void *checked_malloc(size_t size)
{
    void *p;
    p = malloc(size);
    if (p == NULL)
    {
        perror("malloc");
        exit(1);
    }
    return p;
}

char *uint32_to_str(uint32_t i)
{
    int length = snprintf(NULL, 0, "%lu", (unsigned long)i); // pretend to print to a string to get length
    char *str = checked_malloc(length + 1);                  // allocate space for the actual string
    snprintf(str, length + 1, "%lu", (unsigned long)i);      // print to string
    return str;
}

// must be in the root directory where "inodes_list" exists
// inodes array must have size of I_NODES
void open_inodes_list(char *inodes, uint32_t *size)
{
    FILE *file = fopen("inodes_list", "rb");
    if (file == NULL)
    {
        perror("open_inodes_list");
        return;
    }
    uint32_t inode;
    char type;
    while (fread(&inode, sizeof(uint32_t), 1, file))
    {
        if (*size > I_NODES)
        {
            printf("Number of inodes exceeds hard limit of %d", I_NODES);
            exit(1);
        }
        fread(&type, sizeof(char), 1, file);
        inodes[inode] = type;
        *size = *size + 1;
    }
    fclose(file);
}

void read_directory(uint32_t root)
{
    char *root_dir = uint32_to_str(root);
    FILE *file = fopen(root_dir, "rb");
    free(root_dir);
    if (file == NULL)
    {
        perror("Read directory");
        return;
    }
    uint32_t inode;
    char type;
    // read the int value of file
    while (fread(&inode, sizeof(uint32_t), 1, file))
    {
        char filename[NUM_CHARS + 1];
        fread(filename, sizeof(char), NUM_CHARS, file);
        filename[NUM_CHARS] = '\0';
        printf("%u %s\n", inode, filename);
    }
    fclose(file);
}

void cd(char *dir, uint32_t *curr_dir, char *inodes)
{
    char *dir_str = uint32_to_str(*curr_dir);
    FILE *file = fopen(dir_str, "rb");
    free(dir_str);
    uint32_t inode;
    char type;
    if (file == NULL)
        perror("cd");
    bool found = false; // track if we even found a corresponding place to cd into
    while (fread(&inode, sizeof(uint32_t), 1, file))
    {
        char filename[NUM_CHARS + 1];
        fread(filename, sizeof(char), NUM_CHARS, file);
        filename[NUM_CHARS] = '\0';
        if (strcmp(filename, dir) == 0)
        {
            found = true;
            // if trying to cd into a file, notify that's not possible
            if (inodes[inode] == 'f')
                printf("\033[33mError: Can't change directory into file\033[0m\n");
            else
                *curr_dir = inode;
        }
    }
    if (file != NULL)
        fclose(file);
    if (!found)
        printf("\033[33mDirectory for \"%s\" not found. Please enter a valid directory name.\033[0m\n", dir);
}

void ls(uint32_t inode)
{
    read_directory(inode);
}

// String supplied must be of length NUM_CHARS
void null_truncate(char *str)
{
    size_t len = strlen(str);
    for (int i = len; i <= NUM_CHARS; ++i)
    {
        str[i] = '\0';
    }
}

void mkdir(char *dir, uint32_t *curr_dir, uint32_t *size, char *inodes)
{
    char *curr_dir_str = uint32_to_str(*curr_dir);
    FILE *file = fopen(curr_dir_str, "rb");
    free(curr_dir_str);
    if (strlen(dir) > NUM_CHARS)
    {
        printf("\033[33mDirectory name is too large\033[0m\n");
        return;
    }
    if (file == NULL)
    {
        perror("mkdir");
        return;
    }
    // check that the file doesn't already exist
    uint32_t inode;
    char type;
    while (fread(&inode, sizeof(uint32_t), 1, file))
    {
        char filename[NUM_CHARS + 1];
        fread(filename, sizeof(char), NUM_CHARS, file);
        filename[NUM_CHARS] = '\0';
        if (strcmp(filename, dir) == 0)
        {
            printf("\033[33mError: Directory name already in use\033[0m\n");
            return;
        }
    }
    fclose(file);

    // directory doesn't exist, we can now create it in the virtual directory
    curr_dir_str = uint32_to_str(*curr_dir);
    file = fopen(curr_dir_str, "ab");
    free(curr_dir_str);
    fwrite(size, sizeof(uint32_t), 1, file);
    char buffer[NUM_CHARS + 1];
    strcpy(buffer, dir);
    null_truncate(buffer);
    fwrite(buffer, sizeof(char), NUM_CHARS, file);
    inodes[*size] = 'd';
    fclose(file);

    // update inodes_list to reflect that the file exists
    file = fopen("inodes_list", "ab");
    fwrite(size, sizeof(uint32_t), 1, file);
    char filetype = 'd';
    fwrite(&filetype, sizeof(char), 1, file);
    fclose(file);

    // add directory inode file to root directory
    char *inode_dir_str = uint32_to_str(*size);
    file = fopen(inode_dir_str, "ab");
    free(inode_dir_str);
    if (file == NULL)
    {
        perror("mkdir");
        return;
    }
    // create . entry
    char *curr_dir_dots = ".";
    char *parent_dir_dots = "..";
    strcpy(buffer, curr_dir_dots);
    null_truncate(buffer);
    fwrite(size, sizeof(uint32_t), 1, file);
    fwrite(buffer, sizeof(char), NUM_CHARS, file);

    // create .. entry
    memset(buffer, 0, NUM_CHARS);
    strcpy(buffer, parent_dir_dots);
    null_truncate(buffer);
    fwrite(curr_dir, sizeof(uint32_t), 1, file);
    fwrite(buffer, sizeof(char), NUM_CHARS, file);
    fclose(file);
    *size = *size + 1;
}

void touch(char *target, uint32_t *curr_dir, uint32_t *size, char *inodes)
{
    char *curr_dir_str = uint32_to_str(*curr_dir);
    FILE *file = fopen(curr_dir_str, "rb");
    if (strlen(target) > NUM_CHARS)
    {
        printf("\033[33mFilename is too large (>%d characters)\033[0m\n", NUM_CHARS);
        return;
    }
    if (file == NULL)
        perror("touch");
    // check that the file doesn't already exist
    uint32_t inode;
    char type;
    while (fread(&inode, sizeof(uint32_t), 1, file))
    {
        char filename[NUM_CHARS + 1];
        fread(filename, sizeof(char), 32, file);
        filename[NUM_CHARS] = '\0'; // need to account for the fact that we can have 32 chars with a null after
        if (strcmp(filename, target) == 0)
        {
            printf("\033[33mError: Filename already in use\033[0m\n");
            return;
        }
    }
    fclose(file);

    // file doesn't exist, we can now create it in the virtual directory
    file = fopen(curr_dir_str, "ab");
    free(curr_dir_str);
    fwrite(size, sizeof(uint32_t), 1, file);
    char buffer[NUM_CHARS + 1];
    strcpy(buffer, target);
    null_truncate(buffer);
    fwrite(buffer, sizeof(char), NUM_CHARS, file);
    inodes[*size] = 'f';
    fclose(file);

    // update inodes_list to reflect that the file exists
    file = fopen("inodes_list", "ab");
    fwrite(size, sizeof(uint32_t), 1, file);
    char filetype = 'f';
    fwrite(&filetype, sizeof(char), 1, file);
    fclose(file);
    *size = *size + 1;
}

int main(int argc, char *argv[])
{
    validate_params(argc, argv);
    char *filename = argv[DIR_NAME_POS];
    char inodes[I_NODES];
    uint32_t curr_dir = 0;
    // open the directory specified in argv
    DIR *dir = opendir(filename);
    if (dir == NULL)
    {
        perror("Check directory");
        exit(1);
    }
    closedir(dir); // must free directory
    // int fd = open(filename, O_DIRECTORY);
    // if (fd == -1)
    // {
    //     perror("Check directory");
    //     exit(1);
    // }

    // directory located
    int success = chdir(filename);
    if (success != 0)
    {
        perror("Changing directory");
        exit(1);
    }

    uint32_t size = 0;
    open_inodes_list(inodes, &size);

    char *line = NULL;
    size_t length;

    printf("$ ");
    char *token = NULL;
    const char *delim = " \n\t\r";
    while (getline(&line, &length, stdin) > 0)
    {
        bool cd_ = false, ls_ = false, mkdir_ = false, touch_ = false;
        token = strtok(line, delim);
        while (token != NULL)
        {
            if (strcmp(token, "^D") == 0 || strcmp(token, "exit") == 0)
            {
                free(token);
                exit(0);
            }
            else if (strcmp(token, "cd") == 0)
                cd_ = true;
            else if (strcmp(token, "ls") == 0)
                ls(curr_dir);
            else if (strcmp(token, "mkdir") == 0)
                mkdir_ = true;
            else if (strcmp(token, "touch") == 0)
                touch_ = true;
            else if (cd_)
            {
                cd(token, &curr_dir, inodes);
            }
            else if (mkdir_)
                mkdir(token, &curr_dir, &size, inodes);
            else if (touch_) // note this supports making multiple files at once
                touch(token, &curr_dir, &size, inodes);
            else
            {
                printf("\033[33m\"%s\" is not recognized as a command\033[0m\n", token);
                break;
            }
            token = strtok(NULL, delim);
        }
        printf("$ ");
    }
    free(line);
    free(token);
}