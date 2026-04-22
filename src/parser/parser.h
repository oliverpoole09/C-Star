#ifndef PARSER_H
#define PARSER_H
#include "../tokenizer/tokenizer.h"

// All Binary Operators (+, -, *, /, etc.)
typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV
} BinOp;

// Types of Data (Constants)
typedef enum {
    DATA_INT
} DataType;

// Types of Expressions (Constants)
typedef enum {
    EXPR_INT_LIT,
    EXPR_VAR,
    EXPR_BINOP
} ExprType;

// Integer Literal Expression Structure
typedef struct {
    int value;
} IntLiteralExpr;

// Variable Expression Structure
typedef struct {
    Token ident;
} VarExpr;

// Forward declaration of ExprNode so BinOpExpr can point to it
typedef struct ExprNode ExprNode;

// Binary Operator Expression Structure
typedef struct {
    BinOp op;
    ExprNode *left;
    ExprNode *right;
} BinOpExpr;

// Types of data found in Expression Node
typedef union {
    IntLiteralExpr int_lit;
    VarExpr var;
    BinOpExpr bin_op;
} ExprData;

// Expression Node Structure
struct ExprNode {
    ExprType type;
    ExprData data;
};

// Var Decleration Node Structure
typedef struct {
    DataType data_type;
    Token ident;
    ExprNode expr;
} VarDeclNode;

// Reassignment Node Structure
typedef struct {
    Token ident;
    ExprNode expr;
} ReAssignNode;

// Exit Node Structure
typedef struct {
    ExprNode expr;
} ExitNode;

// Types of Data found in a Node
typedef union {
    ExitNode exit;
    VarDeclNode var_decl;
    ReAssignNode re_assign;
} NodeData;

// Types of Nodes (Constants)
typedef enum {
    NODE_EXIT,
    NODE_VAR_DECL,
    NODE_REASSIGN
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

// Parse Reassignment Function
ReAssignNode parse_reassign(Token *tokens, int *i);

// Parse Function (Turn Tokens to Nodes)
Node *parse(Token *tokens, int token_count, int *count);

// print nodes function for debugging
void print_nodes(Node *nodes, int count);

#endif