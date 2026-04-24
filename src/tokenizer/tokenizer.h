#ifndef TOKENIZE_H
#define TOKENIZE_H

// Enum of all TokenTypes that are avalible
typedef enum {
    INT_LIT,
    STR_LIT,
    SEMICOLON,
    LEFT_PAREN,
    RIGHT_PAREN,
    FILE_END,
    DT_INT,
    DT_STR,
    IDENT,
    EQUAL,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    LEFT_CURL,
    RIGHT_CURL,
    COMMA,
    RETURN
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