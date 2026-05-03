#include "generation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// VarEntry Structure (holds var name, depth in stack, and data type)
typedef struct {
    char name[256];
    int depth;
    DataType type;
} VarEntry;

// ParamEntry Structure (holds param name, rbp offset, and data type)
typedef struct {
    char name[256];
    int rbp_offset;
    DataType type;
} ParamEntry;

// StrEntry Structure (holds string label and its value for data section)
typedef struct {
    char label[32];   // e.g. str0, str1
    char value[1024]; // the actual string content
} StrEntry;

static VarEntry var_table[256]; // table of all vars
static int var_count = 0; // holds amount of vars
static int stack_depth = 0; // tracks temporary pushes during expression evaluation

static ParamEntry param_table[64]; // param table for current function
static int param_count = 0; // holds amount of params in current function
static int in_function = 0; // tracks if we are inside a function

static StrEntry str_table[256]; // table of all string literals
static int str_count = 0; // holds amount of string literals

static int strlen_label_count = 0; // unique label counter for strlen loops

// function for looking up variables in table, returns depth or exits on failure
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

// function for looking up var type in table, returns DataType
static DataType lookup_var_type(const char *name) {
    // check var table first
    for (int i = 0; i < var_count; i++) {
        if (strcmp(var_table[i].name, name) == 0)
            return var_table[i].type;
    }
    // check param table
    for (int i = 0; i < param_count; i++) {
        if (strcmp(param_table[i].name, name) == 0)
            return param_table[i].type;
    }
    fprintf(stderr, "Undefined variable: %s\n", name);
    exit(1);
}

// function for looking up params in param table, returns rbp offset or -1 if not found
static int lookup_param(const char *name) {
    for (int i = 0; i < param_count; i++) {
        if (strcmp(param_table[i].name, name) == 0)
            return param_table[i].rbp_offset;
    }
    return -1; // not found
}

// register a string literal in the string table, returns its label index
static int register_string(const char *value) {
    // check if string is already registered to avoid duplicates
    for (int i = 0; i < str_count; i++) {
        if (strcmp(str_table[i].value, value) == 0) {
            return i;
        }
    }
    // register new string
    snprintf(str_table[str_count].label, sizeof(str_table[str_count].label), "str%d", str_count);
    strncpy(str_table[str_count].value, value, sizeof(str_table[str_count].value) - 1);
    return str_count++;
}

// forward declarations
static void emit_func_call(FILE *out, FuncCallNode call);
static void emit_nodes(FILE *out, Node *nodes, int count, int *has_exit);

// collect all string literals from an expression into the string table
static void collect_strings_expr(ExprNode expr) {
    if (expr.type == EXPR_STR_LIT) {
        register_string(expr.data.str_lit.value);
    }
    // collect string segments from f-string parts
    else if (expr.type == EXPR_FSTR) {
        for (int i = 0; i < expr.data.fstr.part_count; i++) {
            if (expr.data.fstr.parts[i].type == FSTR_STR)
                register_string(expr.data.fstr.parts[i].str);
        }
    }
    else if (expr.type == EXPR_BINOP) {
        collect_strings_expr(*expr.data.bin_op.left);
        collect_strings_expr(*expr.data.bin_op.right);
    }
    else if (expr.type == EXPR_FUNC_CALL) {
        for (int i = 0; i < expr.data.func_call.arg_count; i++) {
            collect_strings_expr(expr.data.func_call.args[i]);
        }
    }
}

// collect all string literals from a node list into the string table
static void collect_strings(Node *nodes, int count) {
    for (int i = 0; i < count; i++) {
        switch (nodes[i].type) {
            case NODE_VAR_DECL:
                collect_strings_expr(nodes[i].data.var_decl.expr);
                break;
            case NODE_REASSIGN:
                collect_strings_expr(nodes[i].data.re_assign.expr);
                break;
            case NODE_FUNC_CALL:
                for (int j = 0; j < nodes[i].data.func_call.arg_count; j++)
                    collect_strings_expr(nodes[i].data.func_call.args[j]);
                break;
            case NODE_RETURN:
                collect_strings_expr(nodes[i].data.ret.expr);
                break;
            case NODE_FUNC_DEF:
                collect_strings(nodes[i].data.func_def.body, nodes[i].data.func_def.body_count);
                break;
            default:
                break;
        }
    }
}

// emit the section .data block with all registered strings
static void emit_data_section(FILE *out) {
    if (str_count == 0) return; // no strings, skip section
    fprintf(out, "section .data\n");
    for (int i = 0; i < str_count; i++) {
        // emit each byte of the string manually to handle escape chars
        fprintf(out, "    %s db ", str_table[i].label);
        const char *s = str_table[i].value;
        int first = 1;
        while (*s) {
            if (!first) fprintf(out, ", ");
            // emit raw byte value
            fprintf(out, "%d", (unsigned char)*s);
            s++;
            first = 0;
        }
        fprintf(out, ", 0\n"); // null terminator
        // emit length constant (excludes null terminator)
        fprintf(out, "    %s_len equ $ - %s - 1\n", str_table[i].label, str_table[i].label);
    }
}

// emit the section .bss block for runtime buffers
static void emit_bss_section(FILE *out) {
    fprintf(out, "section .bss\n");
    fprintf(out, "    __int_buf resb 32\n"); // buffer for int to string conversion (32 bytes fits any 64-bit int)
    fprintf(out, "    __read_buf resb 256\n"); // buffer to store user input for read() function
}

// emit the __int_to_str helper function
// input: rdi = integer, rsi = buffer pointer
// output: rdx = length of string written, rsi = buffer pointer
static void emit_int_to_str_helper(FILE *out) {
    fprintf(out, "__int_to_str:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    mov rax, rdi\n"); // number to convert
    fprintf(out, "    mov rcx, rsi\n"); // buffer pointer
    fprintf(out, "    xor r9, r9\n"); // digit count = 0

    // handle 0 specially
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    jnz __its_convert\n");
    fprintf(out, "    mov byte [rcx], 48\n"); // '0'
    fprintf(out, "    mov rdx, 1\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n");

    // extract digits in reverse order
    fprintf(out, "__its_convert:\n");
    fprintf(out, "    mov r8, rcx\n"); // save buffer start
    fprintf(out, "__its_loop:\n");
    fprintf(out, "    test rax, rax\n");
    fprintf(out, "    jz __its_reverse\n");
    fprintf(out, "    xor rdx, rdx\n");
    fprintf(out, "    mov rbx, 10\n");
    fprintf(out, "    div rbx\n"); // rax = quotient, rdx = remainder
    fprintf(out, "    add dl, 48\n"); // convert remainder to ASCII digit
    fprintf(out, "    mov [rcx], dl\n"); // store digit in buffer
    fprintf(out, "    inc rcx\n"); // advance buffer pointer
    fprintf(out, "    inc r9\n"); // increment digit count
    fprintf(out, "    jmp __its_loop\n");

    // reverse the digits in the buffer
    fprintf(out, "__its_reverse:\n");
    fprintf(out, "    mov r10, r8\n"); // start pointer
    fprintf(out, "    dec rcx\n"); // end pointer
    fprintf(out, "__its_rev_loop:\n");
    fprintf(out, "    cmp r10, rcx\n");
    fprintf(out, "    jge __its_done\n");
    fprintf(out, "    mov al, [r10]\n"); // swap bytes at start and end
    fprintf(out, "    mov bl, [rcx]\n");
    fprintf(out, "    mov [r10], bl\n");
    fprintf(out, "    mov [rcx], al\n");
    fprintf(out, "    inc r10\n"); // move start pointer forward
    fprintf(out, "    dec rcx\n"); // move end pointer backward
    fprintf(out, "    jmp __its_rev_loop\n");

    fprintf(out, "__its_done:\n");
    fprintf(out, "    mov rdx, r9\n"); // return length in rdx
    fprintf(out, "    mov rsi, r8\n"); // return buffer start in rsi
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n");
}

// emit a strlen + write syscall for a string pointer already in rsi
static void emit_str_write(FILE *out, int fd) {
    fprintf(out, "    xor rdx, rdx\n"); // length counter = 0
    fprintf(out, "    mov rcx, rsi\n"); // copy pointer for loop
    // generate unique labels to avoid duplicate label errors
    int lbl = strlen_label_count++;
    fprintf(out, ".strlen_loop_%d:\n", lbl);
    fprintf(out, "    cmp byte [rcx], 0\n"); // check for null terminator
    fprintf(out, "    je .strlen_done_%d\n", lbl);
    fprintf(out, "    inc rdx\n"); // increment length
    fprintf(out, "    inc rcx\n"); // advance pointer
    fprintf(out, "    jmp .strlen_loop_%d\n", lbl);
    fprintf(out, ".strlen_done_%d:\n", lbl);
    fprintf(out, "    mov rax, 1\n"); // write syscall
    fprintf(out, "    mov rdi, %d\n", fd); // file descriptor
    fprintf(out, "    syscall\n");
}

// function to evaluate expression and put result in rax
static void eval_expr(FILE *out, ExprNode expr) {
    // if expression is an integer literal
    if (expr.type == EXPR_INT_LIT) {
        // load literal value directly into rax
        fprintf(out, "    mov rax, %d\n", expr.data.int_lit.value);
    }
    // if expression is a string literal
    else if (expr.type == EXPR_STR_LIT) {
        // find the string's label in the string table
        int idx = register_string(expr.data.str_lit.value);
        // load address of string label into rax
        fprintf(out, "    mov rax, %s\n", str_table[idx].label);
    }
    // if expression is an f-string, emit a write for each part
    else if (expr.type == EXPR_FSTR) {
        // for every part in the f-string...
        for (int i = 0; i < expr.data.fstr.part_count; i++) {
            FStrPart part = expr.data.fstr.parts[i];
            // if part is a plain string segment, write it directly
            if (part.type == FSTR_STR) {
                int idx = register_string(part.str);
                fprintf(out, "    mov rax, 1\n"); // write syscall
                fprintf(out, "    mov rdi, 1\n"); // stdout
                fprintf(out, "    mov rsi, %s\n", str_table[idx].label); // string pointer
                fprintf(out, "    mov rdx, %s_len\n", str_table[idx].label); // string length
                fprintf(out, "    syscall\n");
            }
            // if part is an expression, check type and write accordingly
            else if (part.type == FSTR_EXPR) {
                ExprNode *e = part.expr;
                // check if expression is a string variable
                int is_str = 0;
                if (e->type == EXPR_VAR) {
                    DataType t = lookup_var_type(e->data.var.ident.value);
                    if (t == DATA_STR) is_str = 1;
                }
                else if (e->type == EXPR_STR_LIT) {
                    is_str = 1;
                }

                if (is_str) {
                    // load string pointer into rax then write
                    eval_expr(out, *e);
                    fprintf(out, "    mov rsi, rax\n"); // string pointer
                    emit_str_write(out, 1); // write to stdout
                }
                else {
                    // treat as integer, convert and write
                    eval_expr(out, *e);
                    fprintf(out, "    mov rdi, rax\n"); // integer to convert
                    fprintf(out, "    mov rsi, __int_buf\n"); // buffer
                    fprintf(out, "    call __int_to_str\n"); // rsi = buffer, rdx = length
                    fprintf(out, "    mov rax, 1\n"); // write syscall
                    fprintf(out, "    mov rdi, 1\n"); // stdout
                    fprintf(out, "    syscall\n");
                }
            }
        }
    }
    // if expression is a variable
    else if (expr.type == EXPR_VAR) {
        // if inside a function, check param table first
        int param_offset = lookup_param(expr.data.var.ident.value);
        if (in_function && param_offset != -1) {
            // load param value from rbp offset
            fprintf(out, "    mov rax, [rbp + %d]\n", param_offset);
        }
        else {
            // find where variable is (how deep in the stack)
            int depth = lookup_var(expr.data.var.ident.value);
            // account for both variables above us and any temp pushes
            int offset = (var_count - depth + stack_depth) * 8;
            // load vars value directly from the stack into rax
            fprintf(out, "    mov rax, [rsp + %d]\n", offset);
        }
    }
    // if expression is a binary operation (+, -, *, /)
    else if (expr.type == EXPR_BINOP) {
        // evaluate left side, result lands in rax
        eval_expr(out, *expr.data.bin_op.left);
        // push rax to stack to save left side while we evaluate right side
        fprintf(out, "    push rax\n");
        stack_depth++; // temporary push, offsets shift by 8
        // evaluate right side, result lands in rax
        eval_expr(out, *expr.data.bin_op.right);
        // move right side result into rbx
        fprintf(out, "    mov rbx, rax\n");
        // pop left side back into rax
        fprintf(out, "    pop rax\n");
        stack_depth--; // temporary pop, offsets shift back
        // perform the operation
        switch (expr.data.bin_op.op) {
            // add rbx to rax
            case OP_ADD:
                fprintf(out, "    add rax, rbx\n");
                break;
            // subtract rbx from rax
            case OP_SUB:
                fprintf(out, "    sub rax, rbx\n");
                break;
            // multiply rax by rbx (result in rax)
            case OP_MUL:
                fprintf(out, "    imul rax, rbx\n");
                break;
            // divide rax by rbx (result in rax, remainder in rdx)
            case OP_DIV:
                fprintf(out, "    xor rdx, rdx\n"); // clear rdx before division
                fprintf(out, "    idiv rbx\n");
                break;
        }
    }
    // if expression is a function call, emit the call and result lands in rax
    else if (expr.type == EXPR_FUNC_CALL) {
        emit_func_call(out, expr.data.func_call);
    }
}

// emit a function call (push args right to left, call, clean up stack)
static void emit_func_call(FILE *out, FuncCallNode call) {
    // built in function: exit(exit_code);
    if (strcmp(call.ident.value, "exit") == 0) {
        eval_expr(out, call.args[0]); // evaluate exit code into rax
        fprintf(out, "    mov rdi, rax\n");
        fprintf(out, "    mov rax, 60\n");
        fprintf(out, "    syscall\n");
        return;
    }

    // built in function: write(fd, value);
    if (strcmp(call.ident.value, "write") == 0) {
        // first arg is stdout or stderr identifier
        ExprNode fd_expr = call.args[0];
        int fd = 1; // default to stdout
        if (fd_expr.type == EXPR_VAR) {
            if (strcmp(fd_expr.data.var.ident.value, "stderr") == 0) {
                fd = 2; // stderr
            }
        }

        // second arg is the value to write
        ExprNode str_expr = call.args[1];

        // if value is a string literal, use its label and length directly
        if (str_expr.type == EXPR_STR_LIT) {
            int idx = register_string(str_expr.data.str_lit.value);
            fprintf(out, "    mov rax, 1\n"); // write syscall
            fprintf(out, "    mov rdi, %d\n", fd); // file descriptor
            fprintf(out, "    mov rsi, %s\n", str_table[idx].label); // string pointer
            fprintf(out, "    mov rdx, %s_len\n", str_table[idx].label); // string length
            fprintf(out, "    syscall\n");
        }
        // if value is an f-string, eval_expr handles all the writes internally
        else if (str_expr.type == EXPR_FSTR) {
            eval_expr(out, str_expr); // emits all the writes internally
        }
        // if value is an integer expression, convert to string first then write
        else if (str_expr.type == EXPR_INT_LIT || str_expr.type == EXPR_BINOP) {
            eval_expr(out, str_expr); // evaluate integer into rax
            fprintf(out, "    mov rdi, rax\n"); // integer to convert
            fprintf(out, "    mov rsi, __int_buf\n"); // buffer
            fprintf(out, "    call __int_to_str\n"); // rsi = buffer, rdx = length
            fprintf(out, "    mov rax, 1\n"); // write syscall
            fprintf(out, "    mov rdi, %d\n", fd); // file descriptor
            fprintf(out, "    syscall\n");
        }
        // if value is a variable, use type info to determine how to write it
        else if (str_expr.type == EXPR_VAR) {
            DataType var_type = lookup_var_type(str_expr.data.var.ident.value);
            if (var_type == DATA_STR) {
                // load string pointer and write
                eval_expr(out, str_expr);
                fprintf(out, "    mov rsi, rax\n"); // string pointer
                emit_str_write(out, fd);
            }
            else {
                // convert integer variable to string then write
                eval_expr(out, str_expr); // load int into rax
                fprintf(out, "    mov rdi, rax\n"); // integer to convert
                fprintf(out, "    mov rsi, __int_buf\n"); // buffer
                fprintf(out, "    call __int_to_str\n"); // rsi = buffer, rdx = length
                fprintf(out, "    mov rax, 1\n"); // write syscall
                fprintf(out, "    mov rdi, %d\n", fd); // file descriptor
                fprintf(out, "    syscall\n");
            }
        }
        return;
    }
    // built in function: read(fd, prompt);
    else if (strcmp(call.ident.value, "read") == 0) {
        // first arg is stdin (fd = 0)
        // second arg is the prompt string to display before reading
        ExprNode prompt_expr = call.args[1];

        // emit the prompt string first
        if (prompt_expr.type == EXPR_STR_LIT) {
            int idx = register_string(prompt_expr.data.str_lit.value);
            fprintf(out, "    mov rax, 1\n"); // write syscall
            fprintf(out, "    mov rdi, 1\n"); // stdout
            fprintf(out, "    mov rsi, %s\n", str_table[idx].label); // prompt pointer
            fprintf(out, "    mov rdx, %s_len\n", str_table[idx].label); // prompt length
            fprintf(out, "    syscall\n");
        }

        // read input into __read_buf
        fprintf(out, "    mov rax, 0\n"); // read syscall
        fprintf(out, "    mov rdi, 0\n"); // stdin
        fprintf(out, "    mov rsi, __read_buf\n"); // buffer
        fprintf(out, "    mov rdx, 256\n"); // max bytes
        fprintf(out, "    syscall\n");

        // null terminate the input (rax = bytes read, overwrite the newline)
        fprintf(out, "    mov byte [__read_buf + rax - 1], 0\n"); // replace newline with null terminator

        // return pointer to buffer in rax so it can be assigned to a variable
        fprintf(out, "    mov rax, __read_buf\n");
        return;
    }

    // push arguments onto stack in reverse order (right to left)
    for (int i = call.arg_count - 1; i >= 0; i--) {
        eval_expr(out, call.args[i]); // evaluate arg into rax
        fprintf(out, "    push rax\n"); // push arg onto stack
    }
    // call the function
    fprintf(out, "    call %s\n", call.ident.value);
    // clean up args from stack (each arg is 8 bytes)
    if (call.arg_count > 0) {
        fprintf(out, "    add rsp, %d\n", call.arg_count * 8);
    }
    // result is now in rax
}

// emit a function definition as a labeled asm block
static void emit_func_def(FILE *out, FuncDefNode func) {
    // check that function body ends with a return statement
    if (func.body_count == 0 || func.body[func.body_count - 1].type != NODE_RETURN) {
        fprintf(stderr, "Error: function '%s' must end with a return statement\n", func.ident.value);
        exit(1);
    }

    // save outer scope var table and counts
    VarEntry saved_var_table[256];
    int saved_var_count = var_count;
    int saved_stack_depth = stack_depth;
    int saved_param_count = param_count;
    int saved_in_function = in_function;
    memcpy(saved_var_table, var_table, sizeof(var_table));

    // reset var table for function scope
    var_count = 0;
    stack_depth = 0;

    // register params in param table with rbp offsets and types
    // args are pushed right to left so first param is at highest rbp offset
    // rbp+8 = return address, rbp+16 = last arg pushed, rbp+16+(n-1)*8 = first arg
    param_count = 0;
    in_function = 1;
    for (int i = 0; i < func.param_count; i++) {
        strncpy(param_table[i].name, func.params[i].ident.value, 255);
        param_table[i].rbp_offset = 16 + (func.param_count - 1 - i) * 8;
        param_table[i].type = func.params[i].data_type; // store param type
        param_count++;
    }

    // emit function label and set up stack frame
    fprintf(out, "%s:\n", func.ident.value);
    fprintf(out, "    push rbp\n"); // save caller's base pointer
    fprintf(out, "    mov rbp, rsp\n"); // set base pointer to current stack top

    // emit body nodes using shared helper
    int dummy_has_exit = 0;
    emit_nodes(out, func.body, func.body_count, &dummy_has_exit);

    // clear param table and restore outer scope
    param_count = saved_param_count;
    in_function = saved_in_function;
    memcpy(var_table, saved_var_table, sizeof(var_table));
    var_count = saved_var_count;
    stack_depth = saved_stack_depth;
}

// shared node emitter used by both generate() and emit_func_def()
static void emit_nodes(FILE *out, Node *nodes, int count, int *has_exit) {
    for (int i = 0; i < count; i++) {
        switch (nodes[i].type) {
            // if node is a variable decleration
            case NODE_VAR_DECL:
                eval_expr(out, nodes[i].data.var_decl.expr); // evaluate expression and put in rax
                fprintf(out, "    push rax\n"); // push rax (value) to stack to store variable
                // record var name, depth, and type in the variable table
                strncpy(var_table[var_count].name, nodes[i].data.var_decl.ident.value, 255);
                var_table[var_count].depth = var_count + 1;
                var_table[var_count].type = nodes[i].data.var_decl.data_type; // store type
                var_count++; // increase var_count
                break;
            // if node is a reassignment
            case NODE_REASSIGN: {
                eval_expr(out, nodes[i].data.re_assign.expr); // evaluate expression and put in rax
                // find where the variable is on the stack
                int depth = lookup_var(nodes[i].data.re_assign.ident.value);
                int offset = (var_count - depth + stack_depth) * 8;
                // write rax back to the variable's stack slot
                fprintf(out, "    mov [rsp + %d], rax\n", offset);
                break;
            }
            // if node is a function definition, skip (emitted separately after _start)
            case NODE_FUNC_DEF:
                break;
            // if node is a standalone function call
            case NODE_FUNC_CALL:
                emit_func_call(out, nodes[i].data.func_call); // emit call, result in rax
                break;
            // if node is a return statement
            case NODE_RETURN:
                if (!in_function) {
                    fprintf(stderr, "Error: return statement outside of function\n");
                    exit(1);
                }
                eval_expr(out, nodes[i].data.ret.expr); // result in rax
                fprintf(out, "    mov rsp, rbp\n"); // restore rsp, discarding all local vars
                fprintf(out, "    pop rbp\n"); // restore caller's base pointer
                fprintf(out, "    ret\n"); // return to caller
                break;
        }
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

    // check that a main function exists
    int has_main = 0;
    for (int i = 0; i < count; i++) {
        if (nodes[i].type == NODE_FUNC_DEF &&
            strcmp(nodes[i].data.func_def.ident.value, "main") == 0) {
            has_main = 1;
            break;
        }
    }
    if (!has_main) {
        fprintf(stderr, "Error: no main function defined\n");
        exit(1);
    }

    // create and open temporary .asm file
    FILE *out = fopen(asm_path, "w");

    // first pass: collect all string literals into the string table
    collect_strings(nodes, count);

    // emit section .data with all string literals
    emit_data_section(out);

    // emit section .bss with runtime buffers
    emit_bss_section(out);

    // emit section .text
    fprintf(out, "section .text\n");
    fprintf(out, "global _start\n");

    // _start calls main and provides fallback exit in case main returns
    fprintf(out, "_start:\n");
    fprintf(out, "    call main\n"); // call user's main function
    fprintf(out, "    mov rax, 60\n"); // fallback exit syscall
    fprintf(out, "    mov rdi, 0\n"); // exit code 0
    fprintf(out, "    syscall\n");

    // emit all function definitions
    for (int i = 0; i < count; i++) {
        if (nodes[i].type == NODE_FUNC_DEF) {
            emit_func_def(out, nodes[i].data.func_def);
        }
    }

    // emit the __int_to_str helper function
    emit_int_to_str_helper(out);

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

    // remove temporary files (only keeps source and executable)
    remove(asm_path);
    remove(obj_path);
}