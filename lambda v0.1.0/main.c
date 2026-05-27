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
    OP_PUSH_INT,
    OP_STORE_VAR,
    OP_LOAD_VAR,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_OUTPRINT,
    OP_INPRINT,
    OP_HALT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_GT
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

// Compiler Label and Pending Goto Tables
typedef struct {
    char *name;
    int address;
} Label;

Label labels[256];
int label_count = 0;

typedef struct {
    char *label_name;
    int placeholder_offset;
} PendingGoto;

PendingGoto pending_gotos[256];
int pending_goto_count = 0;

int find_label(const char *name) {
    for (int i = 0; i < label_count; i++) {
        if (strcmp(labels[i].name, name) == 0) {
            return labels[i].address;
        }
    }
    return -1;
}

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

void emit_short(uint16_t val) {
    emit_byte((val >> 8) & 0xFF);
    emit_byte(val & 0xFF);
}

void emit_int(int val) {
    emit_byte((val >> 24) & 0xFF);
    emit_byte((val >> 16) & 0xFF);
    emit_byte((val >> 8) & 0xFF);
    emit_byte(val & 0xFF);
}

// Helper to check if a string is a comparison or math operator
// Returns the operator index, or -1 if not found.
int find_binary_op(char *expr, char **out_op, int *out_op_len) {
    char *ops2[] = {"==", "!=", "<=", ">="};
    for (int i = 0; i < 4; i++) {
        char *p = strstr(expr, ops2[i]);
        if (p) {
            *out_op = p;
            *out_op_len = 2;
            return i; // 0: ==, 1: !=, 2: <=, 3: >=
        }
    }
    char *ops1[] = {"<", ">", "+", "-", "*", "/"};
    for (int i = 0; i < 6; i++) {
        char *p = strchr(expr, ops1[i][0]);
        if (p) {
            *out_op = p;
            *out_op_len = 1;
            return i + 4; // 4: <, 5: >, 6: +, 7: -, 8: *, 9: /
        }
    }
    return -1;
}

// Compile expression helper
int compile_expression(char *expr) {
    expr = trim(expr);
    if (expr[0] == '\0') return 0;

    // 1. String literal
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
    
    // 2. Fallback !
    if (strcmp(expr, "!") == 0) {
        int var_idx = get_variable_index("!");
        emit_byte(OP_LOAD_VAR);
        emit_byte(var_idx);
        return 1;
    } 

    // 3. Check for binary operator
    char *op_ptr = NULL;
    int op_len = 0;
    int op_id = find_binary_op(expr, &op_ptr, &op_len);
    if (op_id != -1) {
        *op_ptr = '\0';
        char *left = trim(expr);
        char *right = trim(op_ptr + op_len);

        if (!compile_expression(left) || !compile_expression(right)) {
            return 0;
        }

        switch (op_id) {
            case 0: emit_byte(OP_EQ); break;
            case 1: emit_byte(OP_NE); break;
            case 4: emit_byte(OP_LT); break;
            case 5: emit_byte(OP_GT); break;
            case 6: emit_byte(OP_ADD); break;
            case 7: emit_byte(OP_SUB); break;
            case 8: emit_byte(OP_MUL); break;
            case 9: emit_byte(OP_DIV); break;
            default:
                fprintf(stderr, "Compiler Error: Unsupported operator id %d\n", op_id);
                return 0;
        }
        return 1;
    }

    // 4. Single Integer
    int int_val;
    if (sscanf(expr, "%d", &int_val) == 1) {
        int is_num = 1;
        for (int i = 0; expr[i] != '\0'; i++) {
            if (!isdigit((unsigned char)expr[i]) && expr[i] != '-' && expr[i] != '+') {
                is_num = 0;
                break;
            }
        }
        if (is_num) {
            emit_byte(OP_PUSH_INT);
            emit_int(int_val);
            return 1;
        }
    }

    // 5. Variable name
    int len = strlen(expr);
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

// Control Flow Compiler State
int if_stack[64];
int if_stack_top = 0;

void push_if(int offset) {
    if (if_stack_top >= 64) {
        fprintf(stderr, "Compiler Error: Too many nested if statements\n");
        exit(1);
    }
    if_stack[if_stack_top++] = offset;
}

int pop_if() {
    if (if_stack_top <= 0) {
        fprintf(stderr, "Compiler Error: Unmatched endif/else\n");
        exit(1);
    }
    return if_stack[--if_stack_top];
}

void patch_offset(int placeholder_offset, int target_value) {
    bytecode[placeholder_offset] = (target_value >> 8) & 0xFF;
    bytecode[placeholder_offset + 1] = target_value & 0xFF;
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
        if (strlen(line) == 0 || line[0] == '#') {
            line = strtok(NULL, "\r\n");
            continue;
        }

        if (strncmp(line, "inprint: ", 9) == 0) {
            char *args = line + 9;
            char *arrow = strstr(args, "->");
            char *var_name = NULL;
            char *prompt_part = NULL;

            if (arrow) {
                *arrow = '\0';
                prompt_part = trim(args);
                var_name = trim(arrow + 2);
            } else {
                prompt_part = trim(args);
                var_name = "!";
            }

            if (prompt_part[0] == '"') {
                char *end = strchr(prompt_part + 1, '"');
                if (end) {
                    *end = '\0';
                    int const_idx = add_constant(prompt_part + 1);
                    int var_idx = get_variable_index(var_name);
                    emit_byte(OP_INPRINT);
                    emit_byte(const_idx);
                    emit_byte(var_idx);
                } else {
                    fprintf(stderr, "Compiler Error: Missing closing quote in inprint statement\n");
                    free(buffer);
                    return 1;
                }
            } else {
                fprintf(stderr, "Compiler Error: Expected string literal for prompt: %s\n", prompt_part);
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
        else if (strncmp(line, "if ", 3) == 0) {
            char *then_ptr = strstr(line, " then:");
            if (!then_ptr) {
                fprintf(stderr, "Compiler Error: Expected ' then:' in if statement: %s\n", line);
                free(buffer);
                return 1;
            }
            *then_ptr = '\0';
            char *expr = trim(line + 3);

            if (!compile_expression(expr)) {
                free(buffer);
                return 1;
            }

            emit_byte(OP_JUMP_IF_FALSE);
            push_if(bytecode_count);
            emit_short(0);
        }
        else if (strcmp(line, "else:") == 0) {
            emit_byte(OP_JUMP);
            int else_jump_placeholder = bytecode_count;
            emit_short(0);

            int if_placeholder = pop_if();
            patch_offset(if_placeholder, bytecode_count);
            push_if(else_jump_placeholder);
        }
        else if (strcmp(line, "endif") == 0) {
            int placeholder = pop_if();
            patch_offset(placeholder, bytecode_count);
        }
        else if (strncmp(line, "label ", 6) == 0) {
            char *label_name = trim(line + 6);
            int len = strlen(label_name);
            int valid = (len > 0);
            for (int i = 0; i < len; i++) {
                if (!isalnum((unsigned char)label_name[i]) && label_name[i] != '_') {
                    valid = 0; break;
                }
            }
            if (!valid) {
                fprintf(stderr, "Compiler Error: Invalid label name: %s\n", label_name);
                free(buffer);
                return 1;
            }
            if (find_label(label_name) != -1) {
                fprintf(stderr, "Compiler Error: Label '%s' already defined\n", label_name);
                free(buffer);
                return 1;
            }
            if (label_count >= 256) {
                fprintf(stderr, "Compiler Error: Label table overflow\n");
                free(buffer);
                return 1;
            }
            labels[label_count].name = strdup(label_name);
            labels[label_count].address = bytecode_count;
            label_count++;
        }
        else if (strncmp(line, "goto ", 5) == 0) {
            char *label_name = trim(line + 5);
            int len = strlen(label_name);
            int valid = (len > 0);
            for (int i = 0; i < len; i++) {
                if (!isalnum((unsigned char)label_name[i]) && label_name[i] != '_') {
                    valid = 0; break;
                }
            }
            if (!valid) {
                fprintf(stderr, "Compiler Error: Invalid goto target name: %s\n", label_name);
                free(buffer);
                return 1;
            }

            int addr = find_label(label_name);
            if (addr != -1) {
                emit_byte(OP_JUMP);
                emit_short(addr);
            } else {
                if (pending_goto_count >= 256) {
                    fprintf(stderr, "Compiler Error: Pending goto table overflow\n");
                    free(buffer);
                    return 1;
                }
                emit_byte(OP_JUMP);
                pending_gotos[pending_goto_count].label_name = strdup(label_name);
                pending_gotos[pending_goto_count].placeholder_offset = bytecode_count;
                pending_goto_count++;
                emit_short(0);
            }
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

    // Resolve pending gotos
    for (int i = 0; i < pending_goto_count; i++) {
        int addr = find_label(pending_gotos[i].label_name);
        if (addr == -1) {
            fprintf(stderr, "Compiler Error: Label '%s' is used but not defined\n", pending_gotos[i].label_name);
            free(buffer);
            for (int j = 0; j < label_count; j++) free(labels[j].name);
            for (int j = 0; j < pending_goto_count; j++) free(pending_gotos[j].label_name);
            return 1;
        }
        patch_offset(pending_gotos[i].placeholder_offset, addr);
    }

    if (bytecode_count == 0 || bytecode[bytecode_count - 1] != OP_HALT) {
        emit_byte(OP_HALT);
    }

    free(buffer);

    // ==========================================
    // VM EXECUTION PHASE
    // ==========================================
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
        for (int i = 0; i < label_count; i++) {
            free(labels[i].name);
        }
        for (int i = 0; i < pending_goto_count; i++) {
            free(pending_gotos[i].label_name);
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
                uint8_t prompt_idx = bytecode[ip++];
                uint8_t var_idx = bytecode[ip++];
                printf("%s", constants[prompt_idx]);
                fflush(stdout);

                // Read user input
                char input_buffer[256] = "";
                if (fgets(input_buffer, sizeof(input_buffer), stdin)) {
                    size_t len = strlen(input_buffer);
                    if (len > 0 && input_buffer[len - 1] == '\n') {
                        input_buffer[len - 1] = '\0';
                    }
                    len = strlen(input_buffer);
                    if (len > 0 && input_buffer[len - 1] == '\r') {
                        input_buffer[len - 1] = '\0';
                    }
                }

                if (variables[var_idx].type == VAL_STR && variables[var_idx].as.str_val != NULL) {
                    free(variables[var_idx].as.str_val);
                }
                variables[var_idx].type = VAL_STR;
                variables[var_idx].as.str_val = strdup(input_buffer);
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
            case OP_JUMP: {
                uint16_t offset = (bytecode[ip] << 8) | bytecode[ip+1];
                ip = offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = (bytecode[ip] << 8) | bytecode[ip+1];
                ip += 2;
                Value val = pop();
                if (val.type != VAL_INT) {
                    fprintf(stderr, "VM Runtime Error: Conditional expression must evaluate to integer\n");
                    clean_up();
                    return 1;
                }
                if (val.as.int_val == 0) {
                    ip = offset;
                }
                break;
            }
            case OP_EQ: {
                Value b = pop();
                Value a = pop();
                Value res;
                res.type = VAL_INT;
                if (a.type == VAL_INT && b.type == VAL_INT) {
                    res.as.int_val = (a.as.int_val == b.as.int_val);
                } else if (a.type == VAL_STR && b.type == VAL_STR) {
                    res.as.int_val = (strcmp(a.as.str_val, b.as.str_val) == 0);
                } else {
                    res.as.int_val = 0;
                }
                push(res);
                break;
            }
            case OP_NE: {
                Value b = pop();
                Value a = pop();
                Value res;
                res.type = VAL_INT;
                if (a.type == VAL_INT && b.type == VAL_INT) {
                    res.as.int_val = (a.as.int_val != b.as.int_val);
                } else if (a.type == VAL_STR && b.type == VAL_STR) {
                    res.as.int_val = (strcmp(a.as.str_val, b.as.str_val) != 0);
                } else {
                    res.as.int_val = 1;
                }
                push(res);
                break;
            }
            case OP_LT: {
                Value b = pop();
                Value a = pop();
                if (a.type != VAL_INT || b.type != VAL_INT) {
                    fprintf(stderr, "VM Runtime Error: Relational operators require integers\n");
                    clean_up();
                    return 1;
                }
                Value res;
                res.type = VAL_INT;
                res.as.int_val = (a.as.int_val < b.as.int_val);
                push(res);
                break;
            }
            case OP_GT: {
                Value b = pop();
                Value a = pop();
                if (a.type != VAL_INT || b.type != VAL_INT) {
                    fprintf(stderr, "VM Runtime Error: Relational operators require integers\n");
                    clean_up();
                    return 1;
                }
                Value res;
                res.type = VAL_INT;
                res.as.int_val = (a.as.int_val > b.as.int_val);
                push(res);
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
