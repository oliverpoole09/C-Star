#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Parse Primary Function (Primary: Integer Lits, Identifier)
static ExprNode parse_primary(Token *tokens, int *i) {
    // if current token type is an int_lit
    if (tokens[*i].type == INT_LIT) {
        int val = atoi(tokens[*i].value); // store token value in val
        (*i)++; // next token
        // return ExprNode with EXPR_INT_LIT typ and a int_lit value of (val)
        return (ExprNode){.type = EXPR_INT_LIT, .data.int_lit = (IntLiteralExpr){.value = val}};
    } 
    // else if current token is an identifier
    else if (tokens[*i].type == IDENT) {
        Token t = tokens[*i]; // store token as new token in t
        (*i)++; // next token
        // return ExprNode with EXPR_VAR type and a variable identifier of (t)
        return (ExprNode){.type = EXPR_VAR, .data.var = (VarExpr){.ident = t}};
    } 
    // if no token type was found, throw error and quit
    else {
        fprintf(stderr, "Expected expression (integer literal or identifier), got token type %d\n", tokens[*i].type);
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

// Parse Expressions Function (Follow pemdas order, parse + and - after.)
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

// Parse Exit Function (Parse Exit Function)
ExitNode parse_exit(Token *tokens, int *i) {
    (*i)++; // skip exit token
    // if no left parenthesis, throw error and quit
    if (tokens[*i].type != LEFT_PAREN) {
        fprintf(stderr, "Expected '(' after 'exit'\n");
        exit(1);
    }
    (*i)++; // skip left parenthesis token
    ExprNode expr = parse_expr(tokens, i); // parse expression
    // if no right parenthesis, throw error and quit
    if (tokens[*i].type != RIGHT_PAREN) {
        fprintf(stderr, "Expected ')' after expression\n");
        exit(1);
    }
    (*i)++; // skip right parenthesis token
    // if no semicolon, throw error and quit
    if (tokens[*i].type != SEMICOLON) {
        fprintf(stderr, "Expected ';' after ')'\n");
        exit(1);
    }
    (*i)++; // skip semicolon token
    return (ExitNode){.expr = expr}; // return ExitNode with expression
}

// Parse Variable Decleration Function (Declare/Assign Vars)
VarDeclNode parse_var_decl(Token *tokens, int *i) {
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
    return (VarDeclNode){.data_type = DATA_INT, .ident = ident, .expr = expr};
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

// Parse Function (Turn Tokens to Nodes)
Node *parse(Token *tokens, int token_count, int *count) {
    size_t capacity = 10; // max amount of nodes
    Node *nodes = malloc(sizeof(Node) * capacity); // allocate space for 10 nodes

    // iterate through every token in tokens
    for (int i = 0; i < token_count;) {
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

        // if we reach FILE_END token, break out of loop (should end regardless)
        if (tokens[i].type == FILE_END) {
            break;
        }
        // if current token is an EXIT token, save new node and parse_exit()
        else if (tokens[i].type == EXIT) {
            nodes[(*count)++] = (Node){.type = NODE_EXIT, .data.exit = parse_exit(tokens, &i)};
        } 
        // if current token is an DT_INT token, save new node and parse_var_decl()
        else if (tokens[i].type == DT_INT) {
            nodes[(*count)++] = (Node){.type = NODE_VAR_DECL, .data.var_decl = parse_var_decl(tokens, &i)};
        }
        // if current token is an IDENT and next is EQUAL, it's a reassignment
        else if (tokens[i].type == IDENT && tokens[i + 1].type == EQUAL) {
            nodes[(*count)++] = (Node){.type = NODE_REASSIGN, .data.re_assign = parse_reassign(tokens, &i)};
        }
        // if token does not match any known tokens, throw error and quit
        else {
            fprintf(stderr, "Unexpected token type %d\n", tokens[i].type);
            exit(1);
        }
    }

    return nodes; // return buffer of nodes
}

// helper to print an expression (used in print_nodes)
static void print_expr(ExprNode expr) {
    if (expr.type == EXPR_INT_LIT)
        printf("IntLiteral(%d)", expr.data.int_lit.value);
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
}

// print nodes function for debugging
void print_nodes(Node *nodes, int count) {
    for (int i = 0; i < count; i++) {
        switch (nodes[i].type) {
            case NODE_EXIT:
                printf("ExitNode { expr: ");
                print_expr(nodes[i].data.exit.expr);
                printf(" }\n");
                break;
            case NODE_VAR_DECL:
                printf("VarDeclNode { type: int, ident: %s, expr: ", nodes[i].data.var_decl.ident.value);
                print_expr(nodes[i].data.var_decl.expr);
                printf(" }\n");
                break;
            case NODE_REASSIGN:
                printf("ReAssignNode { ident: %s, expr: ", nodes[i].data.re_assign.ident.value);
                print_expr(nodes[i].data.re_assign.expr);
                printf(" }\n");
                break;
        }
    }
}