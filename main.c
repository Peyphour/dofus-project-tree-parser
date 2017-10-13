#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <memory.h>

typedef struct {
    unsigned char word1_size, word2_size;
    char *word1, *word2;
    unsigned char u_seq3[6];
    unsigned char u_seq2[20];
    unsigned char u_seq1[4];
    unsigned char component_name_size;
    char *component_name;
    unsigned char u_seq0[4];
    unsigned char hash_size;
    unsigned char *hash;
    int64_t file_size;
    int64_t modified_time;
    unsigned char u_seq[6];
} file_node;

typedef struct {
    unsigned char word1_size, word2_size;
    char *word1, *word2;
    unsigned char u_seq[15];
    unsigned char number_of_nodes;
    unsigned char u_flag;
} dir_node;

typedef struct {
    unsigned char magic_number[3];
    unsigned char u_seq[4];
    unsigned char u_seq1[32];
    unsigned char number_of_nodes;
    unsigned char u_flag;
} root_node;

typedef enum {
    ROOT_NODE,
    DIR_NODE,
    FILE_NODE
} NODE_TYPE;

int current_layer = 0;

void print_file_hash(file_node file_node) {
    for(int j = 0; j < file_node.hash_size; j++) {
        printf("%02x", file_node.hash[j]);
    }
}

void print_tabs() {
    for(int i = 0; i < current_layer; i++)
        printf("\t");
}

dir_node read_next_dir_node(FILE *file) {
    dir_node _dir_node;

    fread(&_dir_node.word1_size, sizeof(unsigned char), 1, file);
    _dir_node.word1_size++;

    _dir_node.word1 = malloc(_dir_node.word1_size * sizeof(char));
    fread(_dir_node.word1, sizeof(char), _dir_node.word1_size, file);

    fread(&_dir_node.word2_size, sizeof(unsigned char), 1, file);
    _dir_node.word2_size++;

    _dir_node.word2 = malloc(_dir_node.word1_size * sizeof(char));
    fread(_dir_node.word2, sizeof(char), _dir_node.word2_size, file);

    fread(_dir_node.u_seq, sizeof(unsigned char), 15, file);
    fread(&_dir_node.number_of_nodes, sizeof(unsigned char), 1, file);
    fread(&_dir_node.u_flag, sizeof(unsigned char), 1, file);

    return _dir_node;
}

file_node read_next_file_node(FILE *file) {
    file_node _file_node;

    fread(&_file_node.word1_size, sizeof(unsigned char), 1, file);
    _file_node.word1_size++;
    _file_node.word1 = malloc(_file_node.word1_size * sizeof(char) + 1);
    fread(_file_node.word1, sizeof(char), _file_node.word1_size, file);

    fread(&_file_node.word2_size, sizeof(unsigned char), 1, file);
    _file_node.word2_size++;
    _file_node.word2 = malloc(_file_node.word2_size * sizeof(char));
    fread(_file_node.word2, sizeof(char), _file_node.word2_size, file);

    fread(_file_node.u_seq3, sizeof(unsigned char), 6, file);
    fread(_file_node.u_seq2, sizeof(unsigned char), 20, file);
    fread(_file_node.u_seq1, sizeof(unsigned char), 4, file);

    fread(&_file_node.component_name_size, sizeof(unsigned char), 1, file);
    _file_node.component_name = malloc(_file_node.component_name_size * sizeof(char));
    fread(_file_node.component_name, sizeof(char), _file_node.component_name_size, file);

    fread(_file_node.u_seq0, sizeof(unsigned char), 4, file);

    fread(&_file_node.hash_size, sizeof(unsigned char), 1, file);
    _file_node.hash = malloc(_file_node.hash_size * sizeof(unsigned char));
    fread(_file_node.hash, sizeof(unsigned char), _file_node.hash_size, file);

    fread(&_file_node.file_size, sizeof(int64_t), 1, file);
    fread(&_file_node.modified_time, sizeof(int64_t), 1, file);

    fread(_file_node.u_seq, sizeof(unsigned char), 6, file);

    return _file_node;
}

NODE_TYPE read_next_node_type(FILE *file) {
    long current_file_position = ftell(file);

    NODE_TYPE type;

    const unsigned char dir_identifier[15] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00
    };

    dir_node dir_node = read_next_dir_node(file);

    if(memcmp(dir_identifier, dir_node.u_seq, 14) == 0) {
        type = DIR_NODE;
    } else {
        type = FILE_NODE;
    }

    fseek(file, current_file_position, SEEK_SET);
    assert(ftell(file) == current_file_position);

    return type;
}

void traverse_directory(dir_node directory, FILE *file) {
    for(int i = 0; i < directory.number_of_nodes; i++) {
        NODE_TYPE next_type = read_next_node_type(file);
        if(next_type == DIR_NODE) {
            print_tabs();
            dir_node dir_node = read_next_dir_node(file);
            printf("--d %s (%s) (%d)\n", dir_node.word1, dir_node.word2, dir_node.number_of_nodes);
            current_layer++;
            traverse_directory(dir_node, file);
            current_layer--;
        } else {
            file_node file_node = read_next_file_node(file);
            print_tabs();
            printf("--f %s (%s) (", file_node.word1, file_node.word2);
            print_file_hash(file_node);
            printf(")\n");
        }
    }
}

int main(int argc, char *argv[]) {

    if(argc < 2) {
        printf("Usage : %s {file_path}\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *file = fopen(argv[1], "r");

    if(file == NULL) {
        printf("Couldn't open file %s !", argv[1]);
        return EXIT_FAILURE;
    }

    root_node header;

    fread(&header, sizeof(root_node), 1, file);

    printf("Number of nodes in root : %d\n", header.number_of_nodes);

    for(int i = 0; i < header.number_of_nodes; i++) {
        NODE_TYPE next_type = read_next_node_type(file);
        if(next_type == DIR_NODE) {
            dir_node dir_node = read_next_dir_node(file);
            print_tabs();
            printf("--d %s (%s) (%d)\n", dir_node.word1, dir_node.word2, dir_node.number_of_nodes);
            current_layer++;
            traverse_directory(dir_node, file);
            current_layer--;
        } else {
            file_node file_node = read_next_file_node(file);
            print_tabs();
            printf("--f %s (%s) (", file_node.word1, file_node.word2);
            print_file_hash(file_node);
            printf(")\n");
        }
    }

    fclose(file);

    return EXIT_SUCCESS;
}