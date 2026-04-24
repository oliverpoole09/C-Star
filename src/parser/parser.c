#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// returns 1 if token is a data type keyword, 0 otherwise
static int is_data_type(TokenType type) {
    return type == DT_INT || type == DT_STR;
}

// converts a TokenType to a DataType
static DataType to_data_type(TokenType type) {
    switch (type) {
        case DT_INT: 
            return DATA_INT;
        case DT_STR: 
            return DATA_STR;
        default:
            fprintf(stderr, "Unknown data type token\n");
            exit(1);
    }
}

// Parse Function Call (forward declaration so parse_primary can use it)
FuncCallNode parse_func_call(Token *tokens, int *i, int consume_semicolon);

// Parse Primary Function (Primary: Data Literals, Identifier, Function Calls, etc.)
static ExprNode parse_primary(Token *tokens, int *i) {
    // if current token type is an integer literal
    if (tokens[*i].type == INT_LIT) {
        int val = atoi(tokens[*i].value); // store token value in val
        (*i)++; // next token
        // return ExprNode with EXPR_INT_LIT type and a int_lit value of (val)
        return (ExprNode){.type = EXPR_INT_LIT, .data.int_lit = (IntLiteralExpr){.value = val}};
    }
    // if current token type is a string literal
    else if (tokens[*i].type == STR_LIT) {
        char *val = tokens[*i].value; // store string value
        (*i)++; // next token
        // return ExprNode with EXPR_STR_LIT type and a str_lit value of (val)
        return (ExprNode){.type = EXPR_STR_LIT, .data.str_lit = (StrLiteralExpr){.value = val}};
    }
    // else if current token is an identifier
    else if (tokens[*i].type == IDENT) {
        // if next token is a left paren, it's a function call inside an expression
        if (tokens[*i + 1].type == LEFT_PAREN) {
            FuncCallNode call = parse_func_call(tokens, i, 0); // 0 = don't consume semicolon
            return (ExprNode){.type = EXPR_FUNC_CALL, .data.func_call = call};
        }
        Token t = tokens[*i]; // store token as new token in t
        (*i)++; // next token
        // return ExprNode with EXPR_VAR type and a variable identifier of (t)
        return (ExprNode){.type = EXPR_VAR, .data.var = (VarExpr){.ident = t}};
    }
    // if no token type was found, throw error and quit
    else {
        fprintf(stderr, "Expected expression (integer literal, string literal, or identifier), got token type %d\n", tokens[*i].type);
        exit(1);
    }
}

// Parse Term Function (Follow PEMDAS order, parse * and / first)
static ExprNode parse_term(Token *tokens, int *i) {
    ExprNode left = parse_primary(tokens, i); // parse left side
    // while next token is * or /...
    while (tokens[*i].type == STAR || tokens[*i].type == SLASH) {
        BinOp op = tokens[*i].type == STAR ? OP_MUL : OP_DIV; // store operator
        (*i)++; // skip operator token
        ExprNode right = parse_primary(tokens, i); // parse right side
        // heap allocate left and right so they can be pointed to
        ExprNode *l = malloc(sizeof(ExprNode));
        ExprNode *r = malloc(sizeof(ExprNode));
        *l = left;
        *r = right;
        // combine into a BinOpExpr node
        left = (ExprNode){.type = EXPR_BINOP, .data.bin_op = (BinOpExpr){.op = op, .left = l, .right = r}};
    }
    return left;
}

// Parse Expressions Function (Follow PEMDAS order, parse + and - after)
ExprNode parse_expr(Token *tokens, int *i) {
    ExprNode left = parse_term(tokens, i); // parse left side
    // while next token is + or -...
    while (tokens[*i].type == PLUS || tokens[*i].type == MINUS) {
        BinOp op = tokens[*i].type == PLUS ? OP_ADD : OP_SUB; // store operator
        (*i)++; // skip operator token
        ExprNode right = parse_term(tokens, i); // parse right side
        // heap allocate left and right so they can be pointed to
        ExprNode *l = malloc(sizeof(ExprNode));
        ExprNode *r = malloc(sizeof(ExprNode));
        *l = left;
        *r = right;
        // combine into a BinOpExpr node
        left = (ExprNode){.type = EXPR_BINOP, .data.bin_op = (BinOpExpr){.op = op, .left = l, .right = r}};
    }
    return left;
}

// Parse Variable Decleration Function (Declare/Assign Vars)
VarDeclNode parse_var_decl(Token *tokens, int *i) {
    DataType data_type = to_data_type(tokens[*i].type); // read data type from token
    (*i)++; // skip data type token
    // if no identifier, throw error and quit
    if (tokens[*i].type != IDENT) {
        fprintf(stderr, "Expected identifier after type\n");
        exit(1);
    }
    Token ident = tokens[(*i)++]; // save identifier to 'ident' and move to next token
    // if no equal sign, throw error and quit
    if (tokens[*i].type != EQUAL) {
        fprintf(stderr, "Expected '=' after identifier\n");
        exit(1);
    }
    (*i)++; // skip equal token
    ExprNode expr = parse_expr(tokens, i); // parse expression
    // if no semicolon, throw error and quit
    if (tokens[*i].type != SEMICOLON) {
        fprintf(stderr, "Expected ';' after expression\n");
        exit(1);
    }
    (*i)++; // skip semicolon token
    // return VarDeclNode with data type, identifier, and expression
    return (VarDeclNode){.data_type = data_type, .ident = ident, .expr = expr};
}

// Parse Reassignment Function (Reassign Vars)
ReAssignNode parse_reassign(Token *tokens, int *i) {
    Token ident = tokens[(*i)++]; // save identifier and advance
    (*i)++; // skip equal token
    ExprNode expr = parse_expr(tokens, i); // parse expression
    // if no semicolon, throw error and quit
    if (tokens[*i].type != SEMICOLON) {
        fprintf(stderr, "Expected ';' after expression\n");
        exit(1);
    }
    (*i)++; // skip semicolon token
    return (ReAssignNode){.ident = ident, .expr = expr};
}

// Parse Return Statement
ReturnNode parse_return(Token *tokens, int *i) {
    (*i)++; // skip return token
    ExprNode expr = parse_expr(tokens, i); // parse expression
    // if no semicolon, throw error and quit
    if (tokens[*i].type != SEMICOLON) {
        fprintf(stderr, "Expected ';' after expression\n");
        exit(1);
    }
    (*i)++; // skip semicolon token
    return (ReturnNode){.expr = expr}; // return ReturnNode with expression
}

// Parse Function Call
FuncCallNode parse_func_call(Token *tokens, int *i, int consume_semicolon) {
    Token ident = tokens[(*i)++]; // save identifier (func name) and advance forward
    (*i)++; // skip left parenthesis

    ExprNode temp_args[64]; // temporary list to hold all args
    int arg_count = 0; // keep track of args
    // parse args until we reach a right parenthesis
    while (tokens[*i].type != RIGHT_PAREN) {
        temp_args[arg_count++] = parse_expr(tokens, i); // parse argument expression
        // if next token is a comma, skip it and continue to next arg
        if (tokens[*i].type == COMMA)
            (*i)++;
    }
    (*i)++; // skip right parenthesis

    // if consume_semicolon is true, check for and skip semicolon (standalone call)
    if (consume_semicolon) {
        if (tokens[*i].type != SEMICOLON) {
            fprintf(stderr, "Expected ';' after function call\n");
            exit(1);
        }
        (*i)++; // skip semicolon
    }

    FuncCallNode node; // create new FuncCallNode
    node.ident = ident; // add identifier
    node.arg_count = arg_count; // add arg count
    node.args = malloc(sizeof(ExprNode) * arg_count); // allocate space for args
    // copy args into node
    for (int j = 0; j < arg_count; j++)
        node.args[j] = temp_args[j];
    return node;
}

// Parse Function Definition
FuncDefNode parse_func_def(Token *tokens, int token_count, int *i) {
    int body_count = 0; // tracks amount of nodes in the body
    DataType return_type = to_data_type(tokens[*i].type); // read return type from token
    (*i)++; // skip return type token
    Token ident = tokens[(*i)++]; // grab func name and advance forward
    (*i)++; // skip left paren token

    // Parse Params
    Param params[64]; // list to hold all params
    int param_count = 0; // keep track of params
    // while token is not a right parenthesis (end of params)
    while (tokens[*i].type != RIGHT_PAREN) {
        // read param data type
        DataType param_type = to_data_type(tokens[*i].type);
        (*i)++; // skip data type token
        // grab param ident and store param in params list
        params[param_count++] = (Param){.data_type = param_type, .ident = tokens[(*i)++]};
        // if token is a comma, skip it
        if (tokens[*i].type == COMMA)
            (*i)++;
    }
    (*i)++; // skip right parenthesis
    (*i)++; // skip left curly bracket

    // parse body inside function
    Node *body = parse(tokens, token_count, &body_count, i, RIGHT_CURL);
    (*i)++; // skip RIGHT_CURL

    FuncDefNode node; // create new FuncDefNode
    node.return_type = return_type; // add return type
    node.ident = ident; // add identifier
    node.body = body; // add nodes in body
    node.body_count = body_count; // add amount of nodes in body
    node.param_count = param_count; // add param count
    // for every param in param_count, add it to the FuncDefNode
    for (int j = 0; j < param_count; j++)
        node.params[j] = params[j];
    return node; // return FuncDefNode
}

// Parse Function (Turn Tokens to Nodes)
Node *parse(Token *tokens, int token_count, int *count, int *i, TokenType stop_token) {
    size_t capacity = 10; // max amount of nodes
    Node *nodes = malloc(sizeof(Node) * capacity); // allocate space for 10 nodes

    // iterate through every token in tokens
    for (; tokens[*i].type != stop_token && tokens[*i].type != FILE_END;) {
        // if we reach max capacity for nodes...
        if (*count == capacity) {
            capacity += 10; // increase capacity by 10
            nodes = realloc(nodes, sizeof(Node) * capacity); // reallocate for 10 more nodes
            // if reallocation fails, throw error and exit
            if (nodes == NULL) {
                fprintf(stderr, "Reallocation for nodes failed.\n");
                exit(1);
            }
        }

        // if current token is a data type and 2nd next is LEFT_PAREN, it's a function definition
        if (is_data_type(tokens[*i].type) && tokens[*i + 2].type == LEFT_PAREN) {
            nodes[(*count)++] = (Node){.type = NODE_FUNC_DEF, .data.func_def = parse_func_def(tokens, token_count, i)};
        }
        // if current token is a data type, it's a variable declaration
        else if (is_data_type(tokens[*i].type)) {
            nodes[(*count)++] = (Node){.type = NODE_VAR_DECL, .data.var_decl = parse_var_decl(tokens, i)};
        }
        // if current token is IDENT and next is LEFT_PAREN, it's a standalone function call
        else if (tokens[*i].type == IDENT && tokens[*i + 1].type == LEFT_PAREN) {
            nodes[(*count)++] = (Node){.type = NODE_FUNC_CALL, .data.func_call = parse_func_call(tokens, i, 1)};
        }
        // if current token is IDENT and next is EQUAL, it's a reassignment
        else if (tokens[*i].type == IDENT && tokens[*i + 1].type == EQUAL) {
            nodes[(*count)++] = (Node){.type = NODE_REASSIGN, .data.re_assign = parse_reassign(tokens, i)};
        }
        // if current token is RETURN, it's a return statement
        else if (tokens[*i].type == RETURN) {
            nodes[(*count)++] = (Node){.type = NODE_RETURN, .data.ret = parse_return(tokens, i)};
        }
        // if token does not match any known tokens, throw error and quit
        else {
            fprintf(stderr, "Unexpected token type %d\n", tokens[*i].type);
            exit(1);
        }
    }

    return nodes; // return buffer of nodes
}

// helper to print an expression (used in print_nodes)
static void print_expr(ExprNode expr) {
    if (expr.type == EXPR_INT_LIT)
        printf("IntLiteral(%d)", expr.data.int_lit.value);
    else if (expr.type == EXPR_STR_LIT)
        printf("StrLiteral(%s)", expr.data.str_lit.value);
    else if (expr.type == EXPR_VAR)
        printf("Var(%s)", expr.data.var.ident.value);
    else if (expr.type == EXPR_BINOP) {
        printf("BinOp(");
        print_expr(*expr.data.bin_op.left);
        switch (expr.data.bin_op.op) {
            case OP_ADD: printf(" + "); break;
            case OP_SUB: printf(" - "); break;
            case OP_MUL: printf(" * "); break;
            case OP_DIV: printf(" / "); break;
        }
        print_expr(*expr.data.bin_op.right);
        printf(")");
    }
    else if (expr.type == EXPR_FUNC_CALL) {
        printf("FuncCall(%s, args: [", expr.data.func_call.ident.value);
        for (int i = 0; i < expr.data.func_call.arg_count; i++) {
            print_expr(expr.data.func_call.args[i]);
            if (i < expr.data.func_call.arg_count - 1)
                printf(", ");
        }
        printf("])");
    }
}

// print nodes function for debugging
void print_nodes(Node *nodes, int count) {
    for (int i = 0; i < count; i++) {
        switch (nodes[i].type) {
            case NODE_VAR_DECL:
                printf("VarDeclNode { type: %s, ident: %s, expr: ",
                    nodes[i].data.var_decl.data_type == DATA_INT ? "int" : "str",
                    nodes[i].data.var_decl.ident.value);
                print_expr(nodes[i].data.var_decl.expr);
                printf(" }\n");
                break;
            case NODE_REASSIGN:
                printf("ReAssignNode { ident: %s, expr: ", nodes[i].data.re_assign.ident.value);
                print_expr(nodes[i].data.re_assign.expr);
                printf(" }\n");
                break;
            case NODE_FUNC_DEF:
                printf("FuncDefNode { ident: %s, params: [", nodes[i].data.func_def.ident.value);
                for (int j = 0; j < nodes[i].data.func_def.param_count; j++) {
                    printf("%s %s",
                        nodes[i].data.func_def.params[j].data_type == DATA_INT ? "int" : "str",
                        nodes[i].data.func_def.params[j].ident.value);
                    if (j < nodes[i].data.func_def.param_count - 1)
                        printf(", ");
                }
                printf("], body:\n");
                print_nodes(nodes[i].data.func_def.body, nodes[i].data.func_def.body_count);
                printf("}\n");
                break;
            case NODE_RETURN:
                printf("ReturnNode { expr: ");
                print_expr(nodes[i].data.ret.expr);
                printf(" }\n");
                break;
            case NODE_FUNC_CALL:
                printf("FuncCallNode { ident: %s, args: [", nodes[i].data.func_call.ident.value);
                for (int j = 0; j < nodes[i].data.func_call.arg_count; j++) {
                    print_expr(nodes[i].data.func_call.args[j]);
                    if (j < nodes[i].data.func_call.arg_count - 1)
                        printf(", ");
                }
                printf("] }\n");
                break;
        }
    }
}