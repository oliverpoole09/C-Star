#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Tokenize Function (Turn Text into Tokens)
Token *tokenize(const char *str, int *count) {
    char buffer[256]; // temp "buffer" to hold keywords/letters in
    int buf_len = 0; // var that keeps track of how long the current keyword is
    
    size_t capacity = 10; // make amount of tokens
    // capacity can be extented (seen later), 10 is the set 1st amount
    // (only really used to allocate a starting amount)
    Token *tokens = malloc(sizeof(Token) * capacity); // allocate space for 10 tokens

    // iterate through every char in the str (file)
    for(int i = 0; i < strlen(str); i++) {
        // if we have reached max capacity for tokens...
        if (*count == capacity) {
            capacity += 10; // extend capacity
            tokens = realloc(tokens, sizeof(Token) * capacity); // re-allocate space for 10 more tokens
            // if reallocation fails, exit with error code 1
            if (tokens == NULL) {
                fprintf(stderr, "Reallocation for tokens failed.\n");
                exit(1); 
            }
        }

        char c = str[i]; // store current char in c
        // if current char is alphabetic (a letter)
        if(isalpha(c) || c == '_') {
            buffer[buf_len++] = c; // push current char to buffer
            i++; // advance to the next char
            // while every char after is alphanumeric...
            while(isalnum(str[i]) || str[i] == '_') {
                buffer[buf_len++] = str[i]; // push char to buffer
                i++; // advance to next char
            }
            i--; // go back a char (we end while loop with i++ so when we're done, we're 1 char ahead)

            buffer[buf_len] = '\0'; // end buffer with null terminator (so strcmp() knows where to stop in buffer)
            // if chars/keyword in buffer == "exit"
            if(strcmp(buffer, "exit") == 0) {
                tokens[(*count)++] = (Token){.type = EXIT, .value = NULL}; // push EXIT token into tokens buffer and increase count
                buf_len = 0; // reset buf_len (kinda clearing the buffer, just starting over from the start again)
                continue;
            }
            // if chars/keyword in buffer == "int"
            else if(strcmp(buffer, "int") == 0) {
                tokens[(*count)++] = (Token){.type = DT_INT, .value = NULL}; // push DT_INT token into tokens buffer and increase count
                buf_len = 0; // reset buf_len
                continue;
            }
            // if chars/keyword in buffer = "return"
            else if (strcmp(buffer, "return") == 0) {
                tokens[(*count)++] = (Token){.type = RETURN, .value = NULL}; // push RETURN token into tokens buffer and increase count
                buf_len = 0; // reset buf_len
                continue;
            }
            // if chars/keyword in buffer doesn't match to anything, assume its a identifier
            else {
                char *ident_name = malloc(strlen(buffer) + 1);
                strcpy(ident_name, buffer); // copy buffer into ident_name var
                tokens[(*count)++] = (Token){.type = IDENT, .value = ident_name}; // push IDENT token into tokens buffer and increase count
                buf_len = 0; // reset buf_len
                continue;
            }
        }
        // if current char is a digit
        else if(isdigit(c)) {
            buffer[buf_len++] = c; // push char to buffer
            i++; // advace to next char
            // while every next char is a digit...
            while(isdigit(str[i])) {
                buffer[buf_len++] = str[i]; // push char to buffer
                i++; // advance to next char
            }
            i--; // go back a char (same reason as before)

            buffer[buf_len] = '\0'; // add null terminator to end of buffer
            tokens[(*count)++] = (Token){.type = INT_LIT, .value = strdup(buffer)}; // push INT_LIT token with value of whatevers int he buffer into the tokens buffer and inc count
            buf_len = 0; // reset buffer/buf_len
            continue;
        }
        // if current char is a left paren
        else if(c == '(') {
            tokens[(*count)++] = (Token){.type = LEFT_PAREN, .value = NULL}; // push LEFT_PARENT token into tokens buffer, inc count
            continue;
        }
        // if current char is a right paren
        else if(c == ')') {
            tokens[(*count)++] = (Token){.type = RIGHT_PAREN, .value = NULL}; // push RIGHT_PARENT token into tokens buffer, inc count
            continue;
        }
        // if current char is a right paren
        else if(c == '=') {
            tokens[(*count)++] = (Token){.type = EQUAL, .value = NULL}; // push EQUAL token into tokens buffer, inc count
            continue;
        }
        // if current char is a plus sign
        else if(c == '+') {
            tokens[(*count)++] = (Token){.type = PLUS, .value = NULL}; // push PLUS token into tokens buffer, inc count
            continue;
        }
        // if current char is a minus sign
        else if(c == '-') {
            tokens[(*count)++] = (Token){.type = MINUS, .value = NULL}; // push MINUS token into tokens buffer, inc count
            continue;
        }
        // if current char is a star sign
        else if(c == '*') {
            tokens[(*count)++] = (Token){.type = STAR, .value = NULL}; // push STAR token into tokens buffer, inc count
            continue;
        }
        // if current char is a slash sign
        else if(c == '/') {
            // if next char is a slash aswell, it's a comment
            if (str[i + 1] == '/') {
                // keep ignoring and skipping forward while str isn't a newline char or a null-terminator
                while (str[i] != '\n' && str[i] != '\0') {
                    i++; // next token
                }
            }
            // else if next char is a star, it's a multi-line comment
            else if (str[i + 1] == '*') {
                // keep ignoring and skipping forward until another */ is placed to end the multi-line comment
                while (!(str[i] == '*' && str[i + 1] == '/')) {
                    // if file ends before multi-line comment is closed, throw error
                    if (str[i] == '\0') {
                        fprintf(stderr, "Unclosed multi-line comment\n");
                        exit(1);
                    }
                    i++; // next token
                }
                i += 2;
            }
            // else it's a regular slash token
            else {
                tokens[(*count)++] = (Token){.type = SLASH, .value = NULL}; // push SLASH token into tokens buffer, inc count
            }
            continue;
        }
        // if current char is a left curly bracket
        else if (c == '{') {
            tokens[(*count)++] = (Token){.type = LEFT_CURL, .value = NULL}; // push LEFT_CURL token into tokens buffer, inc count
            continue;
        }
        // if current char is a right curly bracket
        else if (c == '}') {
            tokens[(*count)++] = (Token){.type = RIGHT_CURL, .value = NULL}; // push RIGHT_CURL token into tokens buffer, inc count
            continue;
        }
        // if current char is a comma
        else if (c == ',') {
            tokens[(*count)++] = (Token){.type = COMMA, .value = NULL}; // push COMMA token into tokens buffer, inc count
            continue;
        }
        // if current char is a space, ignore it
        else if(isspace(c)) {
            continue;
        }
        // if current char is a semicolon
        else if(c == ';') {
            tokens[(*count)++] = (Token){.type = SEMICOLON, .value = NULL}; // push SEMICOLON token into tokens buffer, inc count
            continue;
        }
    }
    tokens[*count] = (Token){.type = FILE_END, .value = NULL}; // add FILE_END token to tokens buffer so we know where the file ends
    return tokens; // return tokens buffer (MUST FREE LATER)
}

//----------DEBUG----------


// print tokens function for debugging
void print_tokens(Token *tokens) {
    const char *type_names[] = {
        "EXIT", "INT_LIT", "SEMICOLON", "LEFT_PAREN", "RIGHT_PAREN", "FILE_END", "DT_INT", "IDENT", "EQUAL", "PLUS", "MINUS", "STAR", "SLASH", "LEFT_CURL", "RIGHT_CURL", "COMMA", "RETURN"
    };

    for (int i = 0; tokens[i].type != FILE_END; i++) {
        if (tokens[i].value != NULL) {
            printf("Token { type: %s, value: %s }\n", type_names[tokens[i].type], tokens[i].value);
        } else {
            printf("Token { type: %s }\n", type_names[tokens[i].type]);
        }
    }
}