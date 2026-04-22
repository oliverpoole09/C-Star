#ifndef GENERATION_H
#define GENERATION_H
#include "../parser/parser.h"

// Generate ASM Code (Nodes to ASM Code)
void generate(Node *nodes, int count, const char *filename);

#endif