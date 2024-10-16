#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

// Declare the external function
extern char* get_user_input(const char *prompt);

#define MAX_VARIABLES 100  // Maximum number of variables that can be stored
#define MAX_FUNCTIONS 100  // Maximum number of functions that can be stored

// Structure to hold a variable's name and value
typedef struct {
    char name[50];
    char value[100];
} Variable;

// Structure to hold function definitions
typedef struct {
    char name[50];
    char *code;  // Pointer to the function code
} Function;

// Array to store variables
Variable variables[MAX_VARIABLES];
int variable_count = 0;  // Counter for the number of variables stored

// Array to store functions
Function functions[MAX_FUNCTIONS];
int function_count = 0;

// Global variables for GTK+
GtkWidget *main_window = NULL;
GHashTable *widget_table = NULL; // To store widgets by name

// Function declarations
void set_variable(char *name, char *value);
char* get_variable(char *name);
void store_function(char *name, char *code);
char* get_function_code(char *name);
void execute_handler(GtkWidget *widget, gpointer data);
char* trim(char *str);
int evaluate_condition(char *condition);
void interpret(char *code);

// Function to set a variable's value
void set_variable(char *name, char *value) {
    // Check if the variable already exists
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
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

// Function to store a function definition
void store_function(char *name, char *code) {
    functions[function_count].name[0] = '\0';
    strncat(functions[function_count].name, name, sizeof(functions[function_count].name) - 1);
    functions[function_count].code = code; // Store the pointer to the code
    function_count++;
}

// Function to retrieve function code by name
char* get_function_code(char *name) {
    for (int i = 0; i < function_count; i++) {
        if (strcmp(functions[i].name, name) == 0) {
            return functions[i].code;
        }
    }
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

// Function to execute event handlers
void execute_handler(GtkWidget *widget, gpointer data) {
    char *handler_name = (char *)data;

    // Retrieve the function code by name
    char *function_code = get_function_code(handler_name);
    if (function_code) {
        interpret(function_code); // Execute the function code
    } else {
        printf("Error: Function '%s' not found.\n", handler_name);
    }
}

// Function to interpret and execute the NekoLang code
void interpret(char *code) {
    char line[256];      // Buffer to hold each line of code
    char *ptr = code;    // Pointer to traverse the code
    int in_neko_block = 0;  // Flag to check if inside 'neko { }' block

    // Initialize GTK+
    int argc = 0;
    char **argv = NULL;
    gtk_init(&argc, &argv);

    int gui_mode = 0; // Flag to check if GUI commands are used

    // Initialize the widget table
    widget_table = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);

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

        // Skip empty lines and comments
        if (strlen(trimmed_line) == 0 || strncmp(trimmed_line, "//", 2) == 0) continue;

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
            continue;
        }

        // Handle 'purr' command (print output)
        if (strncmp(trimmed_line, "purr", 4) == 0) {
            // Get the message after 'purr'
            char *msg = trimmed_line + 4;
            msg = trim(msg);

            char output[1024] = "";  // Buffer to build the output message
            char *token = strtok(msg, "+");  // Tokenize the message using '+'

            while (token != NULL) {
                token = trim(token);
                if (token[0] == '\"' && token[strlen(token)-1] == '\"') {
                    token[strlen(token)-1] = '\0';
                    strcat(output, token + 1);
                } else {
                    char *variable_value = get_variable(token);
                    if (variable_value) {
                        strcat(output, variable_value);
                    } else {
                        strcat(output, "(undefined)");
                    }
                }
                token = strtok(NULL, "+");
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

            // Create a prompt message
            char prompt[100];
            snprintf(prompt, sizeof(prompt), "Enter value for %s:", var_name);

            // Get input from the user using the external function
            char *input = get_user_input(prompt);

            if (input == NULL || input[0] == '\0') {
                printf("Error: Input for %s is empty.\n", var_name);
                free(input);  // Free allocated memory
                return;
            }

            // Set the variable with the user's input
            set_variable(var_name, input);

            // Free the input string
            free(input);
        }
        // Handle 'window' command
        else if (strncmp(trimmed_line, "window", 6) == 0) {
            gui_mode = 1;

            // Extract window title and name
            char *rest = trimmed_line + 6;
            rest = trim(rest);

            // Syntax: window windowName "Window Title"
            char *window_name = strtok(rest, " ");
            char *title = strtok(NULL, "");
            title = trim(title);

            // Remove quotes from the title
            if (title[0] == '\"' && title[strlen(title)-1] == '\"') {
                title++;
                title[strlen(title)-1] = '\0';
            }

            // Create the window
            main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            gtk_window_set_title(GTK_WINDOW(main_window), title);
            gtk_container_set_border_width(GTK_CONTAINER(main_window), 10);

            // Store the window in the widget table
            g_hash_table_insert(widget_table, strdup(window_name), main_window);

            // Connect the destroy signal
            g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
        }
        // Handle 'button' command
        else if (strncmp(trimmed_line, "button", 6) == 0) {
            // Syntax: button buttonName "Button Label" parentWidget onClick handlerName
            char *rest = trimmed_line + 6;
            rest = trim(rest);

            char *button_name = strtok(rest, " ");
            char *label = strtok(NULL, "\"");
            label = strtok(NULL, "\"");
            char *parent_name = strtok(NULL, " ");
            parent_name = trim(parent_name);
            char *onClick = strtok(NULL, " ");
            char *handler_name = strtok(NULL, "");

            // Remove quotes from label if necessary
            if (label[0] == '\"' && label[strlen(label)-1] == '\"') {
                label++;
                label[strlen(label)-1] = '\0';
            }

            // Create the button
            GtkWidget *button = gtk_button_new_with_label(label);

            // Store the button in the widget table
            g_hash_table_insert(widget_table, strdup(button_name), button);

            // Get the parent widget
            GtkWidget *parent_widget = g_hash_table_lookup(widget_table, parent_name);
            if (parent_widget) {
                gtk_container_add(GTK_CONTAINER(parent_widget), button);
            } else {
                printf("Error: Parent widget '%s' not found.\n", parent_name);
            }

            // Connect the onClick handler
            if (handler_name) {
                // Store the handler name for later execution
                g_signal_connect(button, "clicked", G_CALLBACK(execute_handler), strdup(handler_name));
            }
        }
        // Handle 'function' command
        else if (strncmp(trimmed_line, "function", 8) == 0) {
            // Get the function name
            char *rest = trimmed_line + 8;
            rest = trim(rest);
            char *function_name = strtok(rest, " ");
            char *bracket = strchr(ptr, '{');
            if (bracket) {
                ptr = bracket + 1;  // Move past the opening '{'
                char *code_start = ptr;
                int brace_count = 1;
                while (brace_count > 0 && *ptr != '\0') {
                    if (*ptr == '{') brace_count++;
                    else if (*ptr == '}') brace_count--;
                    ptr++;
                }
                if (brace_count == 0) {
                    size_t code_length = ptr - code_start - 1;
                    char *function_code = (char *)malloc(code_length + 1);
                    strncpy(function_code, code_start, code_length);
                    function_code[code_length] = '\0';

                    // Store the function definition
                    store_function(function_name, function_code);
                } else {
                    printf("Syntax error: Mismatched braces in function '%s'.\n", function_name);
                }
            } else {
                printf("Syntax error: Missing function body for '%s'.\n", function_name);
            }
        }
        // Handle other commands (implement as needed)
        else {
            printf("Unknown command: %s\n", trimmed_line);
        }
    }

    // If GUI mode is enabled, start the GTK+ main loop
    if (gui_mode) {
        gtk_widget_show_all(main_window);
        gtk_main();
    }

    // Clean up the widget table
    g_hash_table_destroy(widget_table);
}

char* read_code_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Could not open file %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *code = (char *)malloc(file_size + 1);
    if (code == NULL) {
        printf("Error: Memory allocation failed.\n");
        fclose(file);
        return NULL;
    }

    fread(code, 1, file_size, file);
    code[file_size] = '\0';
    fclose(file);

    return code;
}

int main(int argc, char *argv[]) {
    // Check if the user passed any arguments
    if (argc < 2) {
        printf("Usage: %s <filename> [--gui]\n", argv[0]);
        return 1;
    }

    // Read the code from the file
    char *code = read_code_from_file(argv[1]);
    if (code == NULL) {
        return 1;
    }

    // Check if the user wants to run in GUI mode
    int gui_mode = 0;
    if (argc == 3 && strcmp(argv[2], "--gui") == 0) {
        gui_mode = 1;
    }

    if (gui_mode) {
        printf("Running in GUI mode...\n");
    } else {
        printf("Running in console mode...\n");
    }

    // Interpret the code (whether in console or GUI mode)
    interpret(code);

    // Free the code buffer after use
    free(code);

    return 0;
}
#ifdef BUILD_NEKO_INTERPRETER
// Main function for the standalone interpreter
int main(int argc, char *argv[]) {
    // Read the code from stdin
    char code[10000];
    size_t len = fread(code, 1, sizeof(code)-1, stdin);
    code[len] = '\0';  // Null-terminate the code string

    // Interpret the code
    interpret(code);

    return 0;
}
#endif  // BUILD_NEKO_INTERPRETER