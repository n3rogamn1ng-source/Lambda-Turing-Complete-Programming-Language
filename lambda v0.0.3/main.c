#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Helper function to trim leading and trailing whitespace/newlines
char *trim(char *str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
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

    // Buffer to hold the 'it' variable (max 256 chars)
    char it_val[256] = "";

    // Parse line by line
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
                printf("%s", start); // Print prompt without newline
                fflush(stdout);

                // Read user input
                if (fgets(it_val, sizeof(it_val), stdin)) {
                    // Trim trailing newline from input
                    size_t len = strlen(it_val);
                    if (len > 0 && it_val[len - 1] == '\n') {
                        it_val[len - 1] = '\0';
                    }
                    len = strlen(it_val);
                    if (len > 0 && it_val[len - 1] == '\r') {
                        it_val[len - 1] = '\0';
                    }
                }
            } else {
                fprintf(stderr, "Error: Missing closing quote in inprint statement\n");
                free(buffer);
                return 1;
            }
        } 
        else if (strncmp(line, "outprint: \"", 11) == 0) {
            char *start = line + 11;
            char *end = strchr(start, '"');
            if (end) {
                *end = '\0';
                printf("%s\n", start);
            } else {
                fprintf(stderr, "Error: Missing closing quote in outprint statement\n");
                free(buffer);
                return 1;
            }
        } 
        else if (strcmp(line, "outprint: !") == 0) {
            printf("%s\n", it_val);
        } 
        else if (strcmp(line, "endf") == 0) {
            break; // Exit the loop and stop execution
        }
        else {
            fprintf(stderr, "Error: Unknown syntax or command: %s\n", line);
            free(buffer);
            return 1;
        }

        line = strtok(NULL, "\r\n");
    }

    free(buffer);
    return 0;
}
