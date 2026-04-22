#ifndef TOKENIZE_H
#define TOKENIZE_H

// Enum of all TokenTypes that are avalible
typedef enum {
    EXIT,
    INT_LIT,
    SEMICOLON,
    LEFT_PAREN,
    RIGHT_PAREN,
    FILE_END,
    DT_INT,
    IDENT,
    EQUAL,
    PLUS,
    MINUS,
    STAR,
    SLASH
} TokenType;

// Struct of a Token
typedef struct {
    TokenType type;
    char *value;
} Token;

// Tokenize Function (Turn Text into Tokens)
Token *tokenize(const char *str, int *count);

// print tokens function for debugging
void print_tokens(Token *tokens);

#endif