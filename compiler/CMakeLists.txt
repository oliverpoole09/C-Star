cmake_minimum_required(VERSION 3.20)

project(c_star)

set(CMAKE_C_STANDARD 11)

add_executable(csc src/main.c src/tokenizer/tokenizer.c src/parser/parser.c src/generation/generation.c)