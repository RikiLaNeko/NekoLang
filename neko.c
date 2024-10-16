#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VARIABLES 100  // Maximum number of variables that can be stored

// Structure to hold a variable's name and value
typedef struct {
    char name[50];
    char value[100];
} Variable;

// Array to store variables
Variable variables[MAX_VARIABLES];
int variable_count = 0;  // Counter for the number of variables stored

// Function to set a variable's value
void set_variable(char *name, char *value) {
    // Check if the variable already exists
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            // Update the existing variable's value
            strcpy(variables[i].value, value);
            return;
        }
    }
    // Add a new variable
    strcpy(variables[variable_count].name, name);
    strcpy(variables[variable_count].value, value);
    variable_count++;
}

// Function to get a variable's value
char* get_variable(char *name) {
    // Search for the variable by name
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].value;
        }
    }
    // Return NULL if the variable is not found
    return NULL;
}

// Function to trim leading and trailing whitespace and semicolons
char* trim(char *str) {
    char *start = str;  // Pointer to the start of the string
    char *end;

    // Trim leading space and tabs
    while(*start == ' ' || *start == '\t') start++;
    if(*start == 0)  // If all spaces, return empty string
        return start;

    // Trim trailing space, newlines, and semicolons
    end = start + strlen(start) - 1;
    while(end > start && (*end == ' ' || *end == '\n' || *end == ';')) end--;
    *(end+1) = '\0';  // Null-terminate the string

    return start;
}

// Function to interpret and execute the NekoLang code
void interpret(char *code) {
    char line[256];      // Buffer to hold each line of code
    char *ptr = code;    // Pointer to traverse the code
    int in_neko_block = 0;  // Flag to check if inside 'neko { }' block

    while (*ptr != '\0') {
        // Read a line from the code
        char *line_ptr = line;
        while (*ptr != '\n' && *ptr != '\0') {
            *line_ptr++ = *ptr++;
        }
        if (*ptr == '\n') ptr++;  // Move past the newline character
        *line_ptr = '\0';         // Null-terminate the line

        // Trim the line and get a pointer to the trimmed line
        char *trimmed_line = trim(line);

        // Skip empty lines
        if (strlen(trimmed_line) == 0) continue;

        // Check for 'neko {' to start the program block
        if (strcmp(trimmed_line, "neko {") == 0 || strcmp(trimmed_line, "neko{") == 0) {
            in_neko_block = 1;
            continue;
        }

        // Check for '}' to end the program block
        if (strcmp(trimmed_line, "}") == 0) {
            in_neko_block = 0;
            continue;
        }

        if (!in_neko_block) {
            // Ignore lines outside of the neko block
            continue;
        }

        // Handle 'purr' command (print output)
        else if (strncmp(trimmed_line, "purr", 4) == 0) {
            // Get the message after 'purr'
            char *msg = trimmed_line + 4;
            msg = trim(msg);

            char output[1024] = "";  // Buffer to build the output message
            char *token = strtok(msg, "+");  // Tokenize the message using '+'

            while (token != NULL) {
                token = trim(token);  // Trim the token

                // Check if the token is a string literal
                if (token[0] == '\"' && token[strlen(token)-1] == '\"') {
                    // Remove the quotes and append to output
                    token++;
                    token[strlen(token)-1] = '\0';
                    strcat(output, token);
                } else {
                    // Assume the token is a variable name
                    char *value = get_variable(token);
                    if (value) {
                        strcat(output, value);
                    } else {
                        // Variable not found
                        printf("Undefined variable: %s\n", token);
                        return;
                    }
                }
                token = strtok(NULL, "+");  // Get the next token
            }
            // Print the final output message
            printf("%s\n", output);
        }
        // Handle 'kitten' command (variable declaration)
        else if (strncmp(trimmed_line, "kitten", 6) == 0) {
            // Get the rest of the line after 'kitten'
            char *rest = trimmed_line + 6;
            rest = trim(rest);

            // Find the '=' character
            char *equals = strchr(rest, '=');
            if (equals) {
                *equals = '\0';  // Split the string at '='
                char *name = rest;
                char *value = equals + 1;
                name = trim(name);    // Trim the variable name
                value = trim(value);  // Trim the value

                // Remove quotes from the value if present
                if (value[0] == '\"' && value[strlen(value)-1] == '\"') {
                    value++;
                    value[strlen(value)-1] = '\0';
                }
                // Set the variable
                set_variable(name, value);
            } else {
                // Syntax error if '=' is not found
                printf("Syntax error in variable declaration.\n");
            }
        }
        // Handle 'meow' command (user input)
        else if (strncmp(trimmed_line, "meow", 4) == 0) {
            // Get the variable name after 'meow'
            char *var_name = trimmed_line + 4;
            var_name = trim(var_name);

            char input[100];  // Buffer to hold user input
            fgets(input, sizeof(input), stdin);  // Get input from the user

            // Remove the newline character from input
            char *newline = strchr(input, '\n');
            if (newline) *newline = '\0';

            // Set the variable with the user's input
            set_variable(var_name, input);
        }
        // Handle unknown commands
        else {
            printf("Unknown command: %s\n", trimmed_line);
        }
    }
}

// Main function
int main(int argc, char *argv[]) {
    // Check if a filename is provided as an argument
    if (argc < 2) {
        printf("Usage: neko <filename>\n");
        return 1;
    }

    // Open the source file
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        printf("Could not open file %s\n", argv[1]);
        return 1;
    }

    // Read the entire code from the file
    char code[10000];
    size_t len = fread(code, 1, sizeof(code)-1, file);
    code[len] = '\0';  // Null-terminate the code string
    fclose(file);

    // Interpret the code
    interpret(code);

    return 0;
}
