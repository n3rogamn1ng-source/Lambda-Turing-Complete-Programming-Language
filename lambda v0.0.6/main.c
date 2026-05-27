#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

// Value types
typedef enum {
    VAL_INT,
    VAL_STR
} ValType;

typedef struct {
    ValType type;
    union {
        int int_val;
        char *str_val;
    } as;
} Value;

// Opcodes
typedef enum {
    OP_PUSH_STR = 1,
    OP_PUSH_VAR_VAL,
    OP_PUSH_INT,
    OP_STORE_VAR,
    OP_LOAD_VAR,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_OUTPRINT,
    OP_INPRINT,
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

// Compiler Symbol Table
char *symbol_table[256];
int symbol_count = 0;

int get_variable_index(const char *name) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i], name) == 0) {
            return i;
        }
    }
    if (symbol_count >= 256) {
        fprintf(stderr, "Compiler Error: Symbol table overflow\n");
        exit(1);
    }
    symbol_table[symbol_count] = strdup(name);
    return symbol_count++;
}

// Add a string to the constant pool and return its index
int add_constant(const char *str) {
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

void emit_int(int val) {
    emit_byte((val >> 24) & 0xFF);
    emit_byte((val >> 16) & 0xFF);
    emit_byte((val >> 8) & 0xFF);
    emit_byte(val & 0xFF);
}

// Compile expression helper
int compile_expression(char *expr) {
    expr = trim(expr);
    if (expr[0] == '"') {
        char *end = strchr(expr + 1, '"');
        if (end) {
            *end = '\0';
            int const_idx = add_constant(expr + 1);
            emit_byte(OP_PUSH_STR);
            emit_byte(const_idx);
            return 1;
        } else {
            fprintf(stderr, "Compiler Error: Missing closing quote in string literal\n");
            return 0;
        }
    } 
    else if (strcmp(expr, "!") == 0) {
        emit_byte(OP_PUSH_VAR_VAL);
        return 1;
    } 
    else {
        // Try parsing math (e.g. 5 + 10)
        int op1, op2;
        char op;
        if (sscanf(expr, "%d %c %d", &op1, &op, &op2) == 3) {
            if (op == '+' || op == '-' || op == '*' || op == '/') {
                emit_byte(OP_PUSH_INT);
                emit_int(op1);
                emit_byte(OP_PUSH_INT);
                emit_int(op2);
                if (op == '+') emit_byte(OP_ADD);
                else if (op == '-') emit_byte(OP_SUB);
                else if (op == '*') emit_byte(OP_MUL);
                else if (op == '/') emit_byte(OP_DIV);
                return 1;
            } else {
                fprintf(stderr, "Compiler Error: Unsupported operator '%c'\n", op);
                return 0;
            }
        } 
        // Try parsing a single integer
        else if (sscanf(expr, "%d", &op1) == 1) {
            emit_byte(OP_PUSH_INT);
            emit_int(op1);
            return 1;
        } 
        // Must be a variable name
        else {
            int len = strlen(expr);
            if (len == 0) return 0;
            for (int i = 0; i < len; i++) {
                if (!isalnum((unsigned char)expr[i]) && expr[i] != '_') {
                    fprintf(stderr, "Compiler Error: Invalid variable name or expression: %s\n", expr);
                    return 0;
                }
            }
            int var_idx = get_variable_index(expr);
            emit_byte(OP_LOAD_VAR);
            emit_byte(var_idx);
            return 1;
        }
    }
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
        else if (strncmp(line, "outprint: ", 10) == 0) {
            char *arg = line + 10;
            if (!compile_expression(arg)) {
                free(buffer);
                return 1;
            }
            emit_byte(OP_OUTPRINT);
        } 
        else if (strncmp(line, "set ", 4) == 0) {
            char *assignment = line + 4;
            char *equals = strchr(assignment, '=');
            if (!equals) {
                fprintf(stderr, "Compiler Error: Expected '=' in assignment: %s\n", line);
                free(buffer);
                return 1;
            }
            *equals = '\0';
            char *var_name = trim(assignment);
            char *expr = trim(equals + 1);

            if (!compile_expression(expr)) {
                free(buffer);
                return 1;
            }

            int var_idx = get_variable_index(var_name);
            emit_byte(OP_STORE_VAR);
            emit_byte(var_idx);
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

    if (bytecode_count == 0 || bytecode[bytecode_count - 1] != OP_HALT) {
        emit_byte(OP_HALT);
    }

    free(buffer);

    // ==========================================
    // VM EXECUTION PHASE
    // ==========================================
    char it_val[256] = "";
    int ip = 0; // Instruction pointer

    // VM Operand Stack
    Value stack[256];
    int stack_top = 0;

    // VM Variables Array
    Value variables[256];
    for (int i = 0; i < 256; i++) {
        variables[i].type = VAL_INT;
        variables[i].as.int_val = 0;
    }

    void push(Value val) {
        if (stack_top >= 256) {
            fprintf(stderr, "VM Runtime Error: Stack overflow\n");
            exit(1);
        }
        stack[stack_top++] = val;
    }

    Value pop() {
        if (stack_top <= 0) {
            fprintf(stderr, "VM Runtime Error: Stack underflow\n");
            exit(1);
        }
        return stack[--stack_top];
    }

    int read_int() {
        int val = (bytecode[ip] << 24) | (bytecode[ip+1] << 16) | (bytecode[ip+2] << 8) | bytecode[ip+3];
        ip += 4;
        return val;
    }

    // Helper for VM memory cleanup
    void clean_up() {
        for (int i = 0; i < constant_count; i++) {
            free(constants[i]);
        }
        for (int i = 0; i < symbol_count; i++) {
            free(symbol_table[i]);
        }
        for (int i = 0; i < 256; i++) {
            if (variables[i].type == VAL_STR && variables[i].as.str_val != NULL) {
                free(variables[i].as.str_val);
            }
        }
    }

    while (1) {
        uint8_t instruction = bytecode[ip++];
        switch (instruction) {
            case OP_PUSH_STR: {
                uint8_t idx = bytecode[ip++];
                Value val;
                val.type = VAL_STR;
                val.as.str_val = constants[idx];
                push(val);
                break;
            }
            case OP_PUSH_VAR_VAL: {
                Value val;
                val.type = VAL_STR;
                val.as.str_val = it_val;
                push(val);
                break;
            }
            case OP_PUSH_INT: {
                int val_int = read_int();
                Value val;
                val.type = VAL_INT;
                val.as.int_val = val_int;
                push(val);
                break;
            }
            case OP_STORE_VAR: {
                uint8_t var_idx = bytecode[ip++];
                Value val = pop();
                if (variables[var_idx].type == VAL_STR && variables[var_idx].as.str_val != NULL) {
                    free(variables[var_idx].as.str_val);
                }
                variables[var_idx].type = val.type;
                if (val.type == VAL_STR) {
                    variables[var_idx].as.str_val = strdup(val.as.str_val);
                } else {
                    variables[var_idx].as.int_val = val.as.int_val;
                }
                break;
            }
            case OP_LOAD_VAR: {
                uint8_t var_idx = bytecode[ip++];
                push(variables[var_idx]);
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
            case OP_ADD: {
                Value b = pop();
                Value a = pop();
                if (a.type != VAL_INT || b.type != VAL_INT) {
                    fprintf(stderr, "VM Runtime Error: Addition operands must be integers\n");
                    clean_up();
                    return 1;
                }
                Value res;
                res.type = VAL_INT;
                res.as.int_val = a.as.int_val + b.as.int_val;
                push(res);
                break;
            }
            case OP_SUB: {
                Value b = pop();
                Value a = pop();
                if (a.type != VAL_INT || b.type != VAL_INT) {
                    fprintf(stderr, "VM Runtime Error: Subtraction operands must be integers\n");
                    clean_up();
                    return 1;
                }
                Value res;
                res.type = VAL_INT;
                res.as.int_val = a.as.int_val - b.as.int_val;
                push(res);
                break;
            }
            case OP_MUL: {
                Value b = pop();
                Value a = pop();
                if (a.type != VAL_INT || b.type != VAL_INT) {
                    fprintf(stderr, "VM Runtime Error: Multiplication operands must be integers\n");
                    clean_up();
                    return 1;
                }
                Value res;
                res.type = VAL_INT;
                res.as.int_val = a.as.int_val * b.as.int_val;
                push(res);
                break;
            }
            case OP_DIV: {
                Value b = pop();
                Value a = pop();
                if (a.type != VAL_INT || b.type != VAL_INT) {
                    fprintf(stderr, "VM Runtime Error: Division operands must be integers\n");
                    clean_up();
                    return 1;
                }
                if (b.as.int_val == 0) {
                    fprintf(stderr, "VM Runtime Error: Division by zero\n");
                    clean_up();
                    return 1;
                }
                Value res;
                res.type = VAL_INT;
                res.as.int_val = a.as.int_val / b.as.int_val;
                push(res);
                break;
            }
            case OP_OUTPRINT: {
                Value val = pop();
                if (val.type == VAL_STR) {
                    printf("%s\n", val.as.str_val);
                } else {
                    printf("%d\n", val.as.int_val);
                }
                break;
            }
            case OP_HALT:
                clean_up();
                return 0;
            default:
                fprintf(stderr, "VM Runtime Error: Unknown opcode 0x%02X at IP %d\n", instruction, ip - 1);
                clean_up();
                return 1;
        }
    }

    return 0;
}
