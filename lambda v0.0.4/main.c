#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

// Opcodes
typedef enum {
    OP_OUTPRINT_LITERAL = 1,
    OP_INPRINT,
    OP_OUTPRINT_VAR,
    OP_HALT
} OpCode;

// Helper function to trim leading and trailing whitespace/newlines
char *trim(char *str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

// Constant pool and bytecode structures
char *constants[256];
int constant_count = 0;

uint8_t bytecode[1024];
int bytecode_count = 0;

// Add a string to the constant pool and return its index
int add_constant(const char *str) {
    // Check for duplicates
    for (int i = 0; i < constant_count; i++) {
        if (strcmp(constants[i], str) == 0) {
            return i;
        }
    }
    if (constant_count >= 256) {
        fprintf(stderr, "Error: Constant pool overflow\n");
        exit(1);
    }
    constants[constant_count] = strdup(str);
    return constant_count++;
}

void emit_byte(uint8_t byte) {
    if (bytecode_count >= 1024) {
        fprintf(stderr, "Error: Bytecode buffer overflow\n");
        exit(1);
    }
    bytecode[bytecode_count++] = byte;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename.lmba>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", argv[1]);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Out of memory\n");
        fclose(file);
        return 1;
    }

    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);

    // ==========================================
    // COMPILER PHASE
    // ==========================================
    char *line = strtok(buffer, "\r\n");
    while (line != NULL) {
        line = trim(line);
        if (strlen(line) == 0) {
            line = strtok(NULL, "\r\n");
            continue;
        }

        if (strncmp(line, "inprint: \"", 10) == 0) {
            char *start = line + 10;
            char *end = strchr(start, '"');
            if (end) {
                *end = '\0';
                int const_idx = add_constant(start);
                emit_byte(OP_INPRINT);
                emit_byte(const_idx);
            } else {
                fprintf(stderr, "Compiler Error: Missing closing quote in inprint statement\n");
                free(buffer);
                return 1;
            }
        } 
        else if (strncmp(line, "outprint: \"", 11) == 0) {
            char *start = line + 11;
            char *end = strchr(start, '"');
            if (end) {
                *end = '\0';
                int const_idx = add_constant(start);
                emit_byte(OP_OUTPRINT_LITERAL);
                emit_byte(const_idx);
            } else {
                fprintf(stderr, "Compiler Error: Missing closing quote in outprint statement\n");
                free(buffer);
                return 1;
            }
        } 
        else if (strcmp(line, "outprint: !") == 0) {
            emit_byte(OP_OUTPRINT_VAR);
        } 
        else if (strcmp(line, "endf") == 0) {
            emit_byte(OP_HALT);
        } 
        else {
            fprintf(stderr, "Compiler Error: Unknown syntax or command: %s\n", line);
            free(buffer);
            return 1;
        }

        line = strtok(NULL, "\r\n");
    }

    // Always emit HALT at the end of the bytecode if it's not already there
    if (bytecode_count == 0 || bytecode[bytecode_count - 1] != OP_HALT) {
        emit_byte(OP_HALT);
    }

    free(buffer);

    // ==========================================
    // VM EXECUTION PHASE
    // ==========================================
    char it_val[256] = "";
    int ip = 0; // Instruction pointer

    while (1) {
        uint8_t instruction = bytecode[ip++];
        switch (instruction) {
            case OP_OUTPRINT_LITERAL: {
                uint8_t idx = bytecode[ip++];
                printf("%s\n", constants[idx]);
                break;
            }
            case OP_INPRINT: {
                uint8_t idx = bytecode[ip++];
                printf("%s", constants[idx]);
                fflush(stdout);

                // Read user input
                if (fgets(it_val, sizeof(it_val), stdin)) {
                    size_t len = strlen(it_val);
                    if (len > 0 && it_val[len - 1] == '\n') {
                        it_val[len - 1] = '\0';
                    }
                    len = strlen(it_val);
                    if (len > 0 && it_val[len - 1] == '\r') {
                        it_val[len - 1] = '\0';
                    }
                }
                break;
            }
            case OP_OUTPRINT_VAR:
                printf("%s\n", it_val);
                break;
            case OP_HALT:
                // Clean up constant pool memory
                for (int i = 0; i < constant_count; i++) {
                    free(constants[i]);
                }
                return 0;
            default:
                fprintf(stderr, "VM Runtime Error: Unknown opcode 0x%02X at IP %d\n", instruction, ip - 1);
                // Clean up constant pool memory
                for (int i = 0; i < constant_count; i++) {
                    free(constants[i]);
                }
                return 1;
        }
    }

    return 0;
}
