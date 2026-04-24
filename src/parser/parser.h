#ifndef PARSER_H
#define PARSER_H
#include "../tokenizer/tokenizer.h"

// Forward Decleration of Node
typedef struct Node Node;

// Forward declaration of ExprNode
typedef struct ExprNode ExprNode;

// All Binary Operators (+, -, *, /, etc.)
typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV
} BinOp;

// Types of Data (Constants)
typedef enum {
    DATA_INT,
    DATA_STR
} DataType;

// Types of Expressions (Constants)
typedef enum {
    EXPR_INT_LIT,
    EXPR_STR_LIT,
    EXPR_FSTR,
    EXPR_VAR,
    EXPR_BINOP,
    EXPR_FUNC_CALL
} ExprType;

// F-String Part Types
typedef enum {
    FSTR_STR,
    FSTR_EXPR
} FStrPartType;

// F-String Part Structure (either a string segment or an expression)
typedef struct {
    FStrPartType type;
    char *str;
    ExprNode *expr;
} FStrPart;

// F-String Expression Structure
typedef struct {
    FStrPart parts[64];
    int part_count;
} FStrExpr;

// Integer Literal Expression Structure
typedef struct {
    int value;
} IntLiteralExpr;

// String Literal Expression Structure
typedef struct {
    char *value;
} StrLiteralExpr;

// Variable Expression Structure
typedef struct {
    Token ident;
} VarExpr;

// Function Paramaters (int x, int y, etc.)
typedef struct {
    DataType data_type;
    Token ident;
} Param;

// Binary Operator Expression Structure
typedef struct {
    BinOp op;
    ExprNode *left;
    ExprNode *right;
} BinOpExpr;

// Function Calling Node Structure (defined before ExprData so it can be embedded)
typedef struct {
    Token ident;
    ExprNode *args;
    int arg_count;
} FuncCallNode;

// Types of data found in Expression Node
typedef union {
    IntLiteralExpr int_lit;
    StrLiteralExpr str_lit;
    FStrExpr fstr;
    VarExpr var;
    BinOpExpr bin_op;
    FuncCallNode func_call;
} ExprData;

// Expression Node Structure
struct ExprNode {
    ExprType type;
    ExprData data;
};

// Return Node Structure
typedef struct {
    ExprNode expr;
} ReturnNode;

// Function Definition Node Structure
typedef struct {
    DataType return_type;
    Token ident;
    Param params[64];
    int param_count;
    Node *body;
    int body_count;
} FuncDefNode;

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

// Types of Data found in a Node
typedef union {
    VarDeclNode var_decl;
    ReAssignNode re_assign;
    FuncCallNode func_call;
    FuncDefNode func_def;
    ReturnNode ret;
} NodeData;

// Types of Nodes (Constants)
typedef enum {
    NODE_VAR_DECL,
    NODE_REASSIGN,
    NODE_FUNC_DEF,
    NODE_FUNC_CALL,
    NODE_RETURN
} NodeType;

// Node Structure
struct Node {
    NodeType type;
    NodeData data;
};

// Parse Function Definition
FuncDefNode parse_func_def(Token *tokens, int token_count, int *i);

// Parse Function Call
FuncCallNode parse_func_call(Token *tokens, int *i, int consume_semicolon);

// Parse Return Statement
ReturnNode parse_return(Token *tokens, int *i);

// Parse Expressions Function
ExprNode parse_expr(Token *tokens, int *i);

// Parse Variable Decleration Function
VarDeclNode parse_var_decl(Token *tokens, int *i);

// Parse Reassignment Function
ReAssignNode parse_reassign(Token *tokens, int *i);

// Parse Function (Turn Tokens to Nodes)
Node *parse(Token *tokens, int token_count, int *count, int *i, TokenType stop_token);

// print nodes function for debugging
void print_nodes(Node *nodes, int count);

#endif