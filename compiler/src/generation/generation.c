#include "generation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// VarEntry Structure (Holds var name and depth in stack)
typedef struct {
    char name[256];
    int depth;
} VarEntry;

static VarEntry var_table[256]; // static var table to hold variables
static int var_count = 0; // holds amount of vars created

// function for looking up variables in table
static int lookup_var(const char *name) {
    // for every variable in table...
    for (int i = 0; i < var_count; i++) {
        // compare var to var in table, if they match...
        if (strcmp(var_table[i].name, name) == 0)
            // return depth of variable in stack
            return var_table[i].depth;
    }
    // if variable wasn't found, it is undefined, so throw error and quit
    fprintf(stderr, "Undefined variable: %s\n", name);
    exit(1);
}

// function to evaluate expression and put result in rax
static void eval_expr(FILE *out, ExprNode expr) {
    // if expression is an integer literal
    if (expr.type == EXPR_INT_LIT) {
        // load literal vaule directly into rax
        fprintf(out, "    mov rax, %d\n", expr.data.int_lit.value);
    } 
    // if expression is a variable
    else if (expr.type == EXPR_VAR) {
        // find where variable is (how deep in the stack)
        int depth = lookup_var(expr.data.var.ident.value);
        // calculate how many bytes above rsp the var sits
        int offset = (var_count - depth) * 8;
        // load vars value directly from the stack into rax
        fprintf(out, "    mov rax, [rsp + %d]\n", offset);
    }
}

// Generate ASM Code (Nodes to ASM Code)
void generate(Node *nodes, int count, const char *filename) {
    // strip path prefix (everything before the last '/') to get bare filename
    const char *base = strrchr(filename, '/');
    base = base ? base + 1 : filename;
 
    // copy bare filename and strip the extension to get the base name
    char base_name[256];
    strncpy(base_name, base, sizeof(base_name));
    char *dot = strrchr(base_name, '.');
    if (dot) *dot = '\0'; // replace '.' with null terminator to cut off extension
 
    // build output file paths using base name
    char asm_path[512], obj_path[512], bin_path[512];
    snprintf(asm_path, sizeof(asm_path), "/tmp/%s.asm", base_name);
    snprintf(obj_path, sizeof(obj_path), "/tmp/%s.o",   base_name);
    int dir_len = base - filename;
    strncpy(bin_path, filename, dir_len);
    bin_path[dir_len] = '\0';
    strncat(bin_path, base_name, sizeof(bin_path) - dir_len - 1);
 
    // create and open temporary .asm file
    FILE *out = fopen(asm_path, "w");

    // standard boiler plate asm start
    fprintf(out, "global _start\n");
    fprintf(out, "_start:\n");

    int has_exit = 0; // check if program already has an exit function (reduce redundancy)

    // for every node in node list
    for (int i = 0; i < count; i++) {
        // switch for every case a node could be
        switch (nodes[i].type) {
            // if node is a variable decleration
            case NODE_VAR_DECL: 
                eval_expr(out, nodes[i].data.var_decl.expr); // evaluate expression and put in rax
                fprintf(out, "    push rax\n"); // push rax (value) to stack to store variable
                // record var name and depth in the variable table
                strncpy(var_table[var_count].name, nodes[i].data.var_decl.ident.value, 255);
                var_table[var_count].depth = var_count + 1;
                var_count++; // increase var_count
                break;
            // if node is a exit function/command
            case NODE_EXIT: 
                eval_expr(out, nodes[i].data.exit.expr); // evaluate expression and put in rax
                // call exit syscall with expression in exit code
                fprintf(out, "    mov rdi, rax\n");
                fprintf(out, "    mov rax, 60\n");
                fprintf(out, "    syscall\n");
                has_exit = 1; // we have an exit now, set it to true
                break;
        }
    }

    // if no exit command was found, add one anyways with exit code 0
    if (!has_exit) {
        fprintf(out, "    mov rax, 60\n");
        fprintf(out, "    mov rdi, 0\n");
        fprintf(out, "    syscall\n");
    }

    fclose(out); // close file

    // compile .asm file with nasm
    char nasm_cmd[2048];
    strcpy(nasm_cmd, "nasm -felf64 ");
    strcat(nasm_cmd, asm_path);
    strcat(nasm_cmd, " -o ");
    strcat(nasm_cmd, obj_path);
    // if nasm failed, throw error and quit
    int nasm_ret = system(nasm_cmd);
    if (nasm_ret != 0) {
        fprintf(stderr, "nasm failed with code %d\n", nasm_ret);
        exit(1);
    }

    // link .o file with ld
    char ld_cmd[2048];
    strcpy(ld_cmd, "ld ");
    strcat(ld_cmd, obj_path);
    strcat(ld_cmd, " -o ");
    strcat(ld_cmd, bin_path);
    // if linking failed, throw error and quit
    int ld_ret = system(ld_cmd);
    if (ld_ret != 0) {
        fprintf(stderr, "ld failed with code %d\n", ld_ret);
        exit(1);
    }
 
    // remove temporary files (only keeps test.cst and executable)
    remove(asm_path);
    remove(obj_path);
}