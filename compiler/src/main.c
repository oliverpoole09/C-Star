#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "./tokenizer/tokenizer.h"
#include "./parser/parser.h"
#include "./generation/generation.h"

// read file contents function
char *read_file(const char *path) {
    FILE *file = fopen(path, "rb"); // open file, use "rb" to read as binary ("r" sometimes breaks on windows, this is why linux is better)
    fseek(file, 0, SEEK_END); // Jump to end of file
    long size = ftell(file); // get file size by getting current location in file
    rewind(file); // jump back to beginning of file so we can read it later
    char *file_buf = malloc(size + 1); // allocate just enough memory for the file contents + \0
    fread(file_buf, 1, size, file); // read file contents into buffer
    file_buf[size] = '\0'; // add null char to end of file
    fclose(file); // close file
    return file_buf; // return buffer
}

int main(int argc, char* argv[]) {
    // if there is not exactly 2 args (csc file.cst), throw error and quit
    if(argc != 2) {
        fprintf(stderr, "Invalid Argument Amount\n");
        fprintf(stderr, "csc <file.cst>\n");
        exit(1);
    }

    char *contents = read_file(argv[1]); // store file contents into contents
    int token_count = 0; // hold amount of tokens in file
    Token *tokens = tokenize(contents, &token_count); // tokenize file contents
    int node_count = 0; // hold amount of nodes in file
    Node *nodes = parse(tokens, token_count, &node_count); // parse tokens
    generate(nodes, node_count, argv[1]); // generate asm code from ast

    //printf("----------Tokens----------\n\n");
    //print_tokens(tokens);
    //printf("\n----------Nodes----------\n\n");
    //print_nodes(nodes, node_count);

    free(contents); // free contents (technically a buffer) otherwise memory leak
    free(tokens); // free tokens values (hidden on the heap) and tokens buffer
    free(nodes); // free node buffer

    return 0;
}