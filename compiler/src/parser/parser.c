#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Parse Expressions Function
ExprNode parse_expr(Token *tokens, int *i) {
    // if current token type is an integer literal
    if (tokens[*i].type == INT_LIT) {
        int val = atoi(tokens[*i].value); // store token value as val
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
        fprintf(stderr, "Expected expression (int literal or identifier), got token type %d\n", tokens[*i].type);
        exit(1);
    }
}

// Parse Exit Function
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

// Parse Variable Decleration Function
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
        // if token does not match any known tokens, throw error and quit
        else {
            fprintf(stderr, "Unexpected token type %d\n", tokens[i].type);
            exit(1);
        }
    }

    return nodes; // return buffer of nodes
}

// print nodes function for debugging
void print_nodes(Node *nodes, int count) {
    for (int i = 0; i < count; i++) {
        switch (nodes[i].type) {
            case NODE_EXIT:
                printf("ExitNode { expr: ");
                if (nodes[i].data.exit.expr.type == EXPR_INT_LIT)
                    printf("IntLiteral(%d)", nodes[i].data.exit.expr.data.int_lit.value);
                else if (nodes[i].data.exit.expr.type == EXPR_VAR)
                    printf("Var(%s)", nodes[i].data.exit.expr.data.var.ident.value);
                printf(" }\n");
                break;
            case NODE_VAR_DECL:
                printf("VarDeclNode { type: int, ident: %s, expr: ", nodes[i].data.var_decl.ident.value);
                if (nodes[i].data.var_decl.expr.type == EXPR_INT_LIT)
                    printf("IntLiteral(%d)", nodes[i].data.var_decl.expr.data.int_lit.value);
                else if (nodes[i].data.var_decl.expr.type == EXPR_VAR)
                    printf("Var(%s)", nodes[i].data.var_decl.expr.data.var.ident.value);
                printf(" }\n");
                break;
        }
    }
}