#ifndef PARSER_H
#define PARSER_H
#include "../tokenizer/tokenizer.h"

// Types of Data (Constants)
typedef enum {
    DATA_INT
} DataType;

// Types of Expressions (Constants)
typedef enum {
    EXPR_INT_LIT,
    EXPR_VAR
} ExprType;

// Integer Literal Expression Structure
typedef struct {
    int value;
} IntLiteralExpr;

// Variable Expression Structure
typedef struct {
    Token ident;
} VarExpr;

// Types of data found in Expression Node
typedef union {
    IntLiteralExpr int_lit;
    VarExpr var;
} ExprData;

// Expression Node Structure
typedef struct {
    ExprType type;
    ExprData data;
} ExprNode;

// Exit Node Structure
typedef struct {
    ExprNode expr;
} ExitNode;

// Var Decleration Node Structure
typedef struct {
    DataType data_type;
    Token ident;
    ExprNode expr;
} VarDeclNode;

// Types of Data found in a Node
typedef union {
    ExitNode exit;
    VarDeclNode var_decl;
} NodeData;

// Types of Nodes (Constants)
typedef enum {
    NODE_EXIT,
    NODE_VAR_DECL
} NodeType;

// Node Structure
typedef struct {
    NodeType type;
    NodeData data;
} Node;

// Parse Expressions Function
ExprNode parse_expr(Token *tokens, int *i);

// Parse Exit Function
ExitNode parse_exit(Token *tokens, int *i);

// Parse Variable Decleration Function
VarDeclNode parse_var_decl(Token *tokens, int *i);

// Parse Function (Turn Tokens to Nodes)
Node *parse(Token *tokens, int token_count, int *count);

// print nodes function for debugging
void print_nodes(Node *nodes, int count);

#endif