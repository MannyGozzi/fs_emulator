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

struct INODE
{
    uint32_t num;
    char type;
    FILE *file;
};

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

// appends the filename to the provided INODE
void get_file(struct INODE* node) {
    FILE* file = fopen(uint32_to_str(node->num), "r");
    if (file != NULL) node->file = file;
    else node->file = NULL;
}

char* get_filename(struct INODE* node) {
    if (node->file == NULL) return "";
    char line[NUM_CHARS];
    int n = fread(line, sizeof(char), NUM_CHARS, node->file);
    return "";
}

// returns numbers of i_nodes, returns -1 on error,
size_t read_inodes(struct INODE *nodes[I_NODES])
{
    FILE *file = fopen("inodes_list", "r");
    size_t index = 0;
    while (1)
    {
        if (file == NULL)
        {
            perror("File not found");
            return -1;
        }
        nodes[index] = checked_malloc(sizeof(struct INODE));
        int n = fread(&(nodes[index]->num), sizeof(uint32_t), 1, file);
        if (n <= 0)
            return index;
        n = fread(&(nodes[index]->type), sizeof(char), 1, file);
        if (n <= 0)
            return index;

        // get file pointer for files
        if (nodes[index]->type == 'f') {
            // set the file pointer in the inode
            get_file(nodes[index]);
        }

        ++index;
    }
    return index - 1;
}

void clean(struct INODE *nodes[I_NODES], size_t size)
{
    for (int i = 0; i < size; ++i)
    {
        free(nodes[i]);
    }
}

void print_nodes(struct INODE *nodes[I_NODES], size_t size) {
    for(int i = 0; i < size; ++i)
    {
        printf("%i, %c\n", nodes[i]->num, nodes[i]->type);
    }
}

// returns true if successful, false if not
bool cd(char* dir, struct INODE *nodes[I_NODES]) {
    // directory located
    int success = chdir(dir);
    if (success != 0)
    {
        printf("Please enter a valid directory\n");
        return false;
    }
    return true;
}

void ls(struct INODE *nodes[I_NODES]) {
    
}

void mkdir(char* dir) {
    
}

void touch(char* filename) {
    
}


int main(int argc, char *argv[])
{
    validate_params(argc, argv);
    char *filename = argv[FILE_NAME_POS];
    struct INODE *nodes[I_NODES];
    uint32_t curr_dir = 0, prev_dir = 0;

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

    size_t size = read_inodes(nodes);
    print_nodes(nodes, size);

    char* line;
    size_t length;
    
    while (getline(&line, &length, stdin) > 0) {
        char* orig_line = line;
        char *token = NULL;
        bool cd_ = false, ls_ = false, mkdir_ = false, touch_ = false;
        while ((token = strsep(&orig_line, " ")) != NULL)
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
                if (cd(token, nodes)) {
                    prev_dir = curr_dir;
                    curr_dir = (uint32_t) atoi(token);
                } 
            }
            else if (ls_) 
                ls(nodes);
            else if (mkdir_) 
                mkdir(token);
            else if (touch_) 
                touch(token);
            else printf("%s is not recognized as a command\n", token);
        }
    }

    clean(nodes, size);
}