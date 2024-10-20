// neko.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Define maximum sizes for various inputs and names
#define INPUT_SIZE 256
#define NAME_SIZE 64
#define VALUE_SIZE 256
#define FUNCTION_CODE_SIZE 4096

// Structure to store a variable
typedef struct Variable {
    char name[NAME_SIZE];
    char value[VALUE_SIZE];
    struct Variable *next;
} Variable;

// Structure to store a function
typedef struct Function {
    char name[NAME_SIZE];
    char *code;
    struct Function *next;
} Function;

// Linked lists for variables and functions
Variable *variables_head = NULL;
Function *functions_head = NULL;

// Structures for voxel management and player
typedef struct Voxel {
    float x, y, z;
    struct Voxel *next;
} Voxel;

Voxel *voxels_head = NULL;

// OpenGL-related global variables
unsigned int VAO = 0, VBO = 0;
GLFWwindow* gl_window = NULL;
GLuint shaderProgram = 0;

// Player position and camera angles
float player_x = 0.0f, player_y = 1.0f, player_z = 5.0f;
float camera_pitch = 0.0f, camera_yaw = -90.0f;

// Flag to indicate if OpenGL has been initialized
int opengl_initialized = 0;

// Verbose mode flag
int verbose = 0;

// Function declarations
char* get_user_input(const char *prompt);
void set_variable(const char *name, const char *value);
char* get_variable(const char *name);
void store_function(const char *name, const char *code);
char* get_function_code(const char *name);
char* trim(char *str);
void interpret(const char *code, int gui_mode);
void neko_window(const char *title, int width, int height);
void neko_draw_scene(); // Only one declaration
void neko_add_block(float x, float y, float z);
void neko_remove_block(float x, float y, float z);
char* read_code_from_file(const char *filename);
void setup_opengl_objects();
GLuint compile_shader(const char* source, GLenum type);
GLuint create_shader_program(const char* vertexSource, const char* fragmentSource);
void cleanup();
int detect_gui_mode(const char *code);
int is_key_pressed(const char *key);
void neko_set_player_position(float x, float y, float z);

// Function implementations

// Function to obtain user input securely
char* get_user_input(const char *prompt) {
    static char input[INPUT_SIZE];
    printf("%s", prompt);
    if (fgets(input, sizeof(input), stdin) != NULL) {
        input[strcspn(input, "\n")] = '\0'; // Remove newline
    }
    return input;
}

// Function to set or update a variable
void set_variable(const char *name, const char *value) {
    Variable *current = variables_head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            strncpy(current->value, value, VALUE_SIZE - 1);
            current->value[VALUE_SIZE - 1] = '\0';
            return;
        }
        current = current->next;
    }
    // Create a new variable if it doesn't exist
    Variable *new_var = (Variable *)malloc(sizeof(Variable));
    if (new_var == NULL) {
        fprintf(stderr, "Memory allocation failed for variable.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(new_var->name, name, NAME_SIZE - 1);
    new_var->name[NAME_SIZE - 1] = '\0';
    strncpy(new_var->value, value, VALUE_SIZE - 1);
    new_var->value[VALUE_SIZE - 1] = '\0';
    new_var->next = variables_head;
    variables_head = new_var;
}

// Function to get a variable's value
char* get_variable(const char *name) {
    Variable *current = variables_head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current->value;
        }
        current = current->next;
    }
    return NULL; // Variable not found
}

// Function to store a function definition
void store_function(const char *name, const char *code) {
    Function *current = functions_head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            free(current->code);
            current->code = strdup(code);
            return;
        }
        current = current->next;
    }
    // Create a new function if it doesn't exist
    Function *new_func = (Function *)malloc(sizeof(Function));
    if (new_func == NULL) {
        fprintf(stderr, "Memory allocation failed for function.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(new_func->name, name, NAME_SIZE - 1);
    new_func->name[NAME_SIZE - 1] = '\0';
    new_func->code = strdup(code);
    if (new_func->code == NULL) {
        fprintf(stderr, "Memory allocation failed for function code.\n");
        free(new_func);
        exit(EXIT_FAILURE);
    }
    new_func->next = functions_head;
    functions_head = new_func;
}

// Function to get a function's code by name
char* get_function_code(const char *name) {
    Function *current = functions_head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current->code;
        }
        current = current->next;
    }
    return NULL; // Function not found
}

// Function to trim leading and trailing whitespace and semicolons
char* trim(char *str) {
    if (str == NULL) return NULL;

    // Trim leading whitespace
    while (*str == ' ' || *str == '\t') str++;

    // Trim trailing whitespace and semicolons
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\n' || *end == ';')) end--;
    *(end + 1) = '\0';

    return str;
}

// Function to compile a shader
GLuint compile_shader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Check for compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        const char *shader_type = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        fprintf(stderr, "Shader Compilation Error (%s):\n%s\n", shader_type, infoLog);
    }
    return shader;
}

// Function to create a shader program
GLuint create_shader_program(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compile_shader(vertexSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compile_shader(fragmentSource, GL_FRAGMENT_SHADER);

    // Create shader program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Check for linking errors
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "Shader Program Linking Error:\n%s\n", infoLog);
    }

    // Delete shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// Function to set up OpenGL objects (e.g., VAO, VBO)
void setup_opengl_objects() {
    // Define vertices for a cube
    float vertices[] = {
        // positions
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f
    };

    // Generate and bind VAO and VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Define vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Unbind VAO and VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Define shader sources
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "}\0";

    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 objectColor;\n"
        "uniform vec3 lightColor;\n"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(lightColor * objectColor, 1.0);\n"
        "}\n\0";

    // Create shader program
    shaderProgram = create_shader_program(vertexShaderSource, fragmentShaderSource);
    opengl_initialized = 1; // Indicate that OpenGL has been initialized
}

// Function to draw the scene (render all voxels)
void neko_draw_scene() {
    if (!opengl_initialized) {
        fprintf(stderr, "OpenGL not initialized. Cannot draw.\n");
        return;
    }

    // Clear the color and depth buffers
    glClearColor(0.5f, 0.7f, 1.0f, 1.0f); // Sky blue background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use the shader program
    glUseProgram(shaderProgram);

    // Define transformation matrices (simplified; consider using GLM for complex transformations)
    float model[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    float view[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        -player_x, -player_y, -player_z, 1
    };
    float projection[16] = {
        1.29904f, 0, 0, 0,
        0, 1.73205f, 0, 0,
        0, 0, -1.002f, -0.2002f,
        0, 0, -1, 0
    };

    // Pass matrices to shader
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc  = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc  = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);

    // Set object and light colors
    GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    GLint lightColorLoc  = glGetUniformLocation(shaderProgram, "lightColor");
    glUniform3f(objectColorLoc, 0.4f, 0.8f, 0.4f); // Green for blocks
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);  // White light

    // Bind VAO and draw each voxel
    glBindVertexArray(VAO);
    Voxel *current = voxels_head;
    while (current != NULL) {
        // Compute model matrix for each voxel
        float model_matrix[16] = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            current->x, current->y, current->z, 1
        };
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model_matrix);

        glDrawArrays(GL_TRIANGLES, 0, 36);
        current = current->next;
    }
    glBindVertexArray(0);
}

// Function to add a block
void neko_add_block(float x, float y, float z) {
    // Check if the block already exists
    Voxel *current = voxels_head;
    while (current != NULL) {
        if (current->x == x && current->y == y && current->z == z) {
            printf("Block already present at (%.1f, %.1f, %.1f).\n", x, y, z);
            return;
        }
        current = current->next;
    }

    // Add a new block
    Voxel *new_voxel = (Voxel *)malloc(sizeof(Voxel));
    if (new_voxel == NULL) {
        fprintf(stderr, "Memory allocation failed for voxel.\n");
        exit(EXIT_FAILURE);
    }
    new_voxel->x = x;
    new_voxel->y = y;
    new_voxel->z = z;
    new_voxel->next = voxels_head;
    voxels_head = new_voxel;

    if (verbose) printf("Block added at (%.1f, %.1f, %.1f).\n", x, y, z);
}

// Function to remove a block
void neko_remove_block(float x, float y, float z) {
    Voxel *current = voxels_head;
    Voxel *prev = NULL;
    while (current != NULL) {
        if (current->x == x && current->y == y && current->z == z) {
            if (prev == NULL) {
                voxels_head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            if (verbose) printf("Block removed at (%.1f, %.1f, %.1f).\n", x, y, z);
            return;
        }
        prev = current;
        current = current->next;
    }
    printf("No block found at (%.1f, %.1f, %.1f).\n", x, y, z);
}

// Function to create an OpenGL window
void neko_window(const char *title, int width, int height) {
    if (!glfwInit()) {
        fprintf(stderr, "GLFW initialization failed.\n");
        exit(EXIT_FAILURE);
    }

    // Configure GLFW for OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE); // Enable double buffering

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Necessary for Mac
#endif

    // Create the GLFW window
    gl_window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!gl_window) {
        fprintf(stderr, "Failed to create GLFW window.\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // Make the OpenGL context current
    glfwMakeContextCurrent(gl_window);

    // Enable V-Sync
    glfwSwapInterval(1);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "GLEW initialization failed.\n");
        glfwDestroyWindow(gl_window);
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // Set the viewport
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(gl_window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    // Setup OpenGL objects (VAO, VBO, Shaders)
    setup_opengl_objects();
}

// Function to check if a key is pressed
int is_key_pressed(const char *key) {
    if (!opengl_initialized || !gl_window) {
        return 0;
    }

    if (strcmp(key, "W") == 0) {
        return glfwGetKey(gl_window, GLFW_KEY_W) == GLFW_PRESS;
    } else if (strcmp(key, "S") == 0) {
        return glfwGetKey(gl_window, GLFW_KEY_S) == GLFW_PRESS;
    } else if (strcmp(key, "A") == 0) {
        return glfwGetKey(gl_window, GLFW_KEY_A) == GLFW_PRESS;
    } else if (strcmp(key, "D") == 0) {
        return glfwGetKey(gl_window, GLFW_KEY_D) == GLFW_PRESS;
    } else if (strcmp(key, "ESC") == 0) {
        return glfwGetKey(gl_window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    }

    return 0;
}

// Function to set the player's position
void neko_set_player_position(float x, float y, float z) {
    player_x = x;
    player_y = y;
    player_z = z;
    if (verbose) printf("Player position updated to (%.2f, %.2f, %.2f)\n", player_x, player_y, player_z);
}

// Function to interpret and execute NekoLang code
void interpret(const char *code, int gui_mode) {
    char line[1024];          // Buffer for each line of code
    const char *ptr = code;   // Pointer to traverse the code
    int in_neko_block = 0;    // Flag to check if inside a 'neko { }' block
    int local_opengl_mode = gui_mode; // Local OpenGL mode based on initial scan

    while (*ptr != '\0') {
        // Read a line of code
        const char *line_start = ptr;
        while (*ptr != '\n' && *ptr != '\0') {
            ptr++;
        }
        size_t len = ptr - line_start;
        if (len >= sizeof(line)) len = sizeof(line) - 1; // Limit line length
        strncpy(line, line_start, len);
        line[len] = '\0';
        if (*ptr == '\n') ptr++; // Skip newline character

        // Trim the line
        char *trimmed_line = trim(line);

        // Skip empty lines and comments
        if (strlen(trimmed_line) == 0 || strncmp(trimmed_line, "//", 2) == 0) continue;

        // Check for 'neko {' to enter the block
        if (strcmp(trimmed_line, "neko {") == 0 || strcmp(trimmed_line, "neko{") == 0) {
            in_neko_block = 1;
            if (verbose) printf("Entering 'neko' block.\n");
            continue;
        }

        // Check for '}' to exit the block
        if (strcmp(trimmed_line, "}") == 0) {
            in_neko_block = 0;
            if (verbose) printf("Exiting 'neko' block.\n");
            continue;
        }

        if (!in_neko_block) {
            continue;
        }

        // Handle 'purr' command (print)
        if (strncmp(trimmed_line, "purr", 4) == 0) {
            char *msg = trim(trimmed_line + 4);
            char output[1024] = "";
            char *token = strtok(msg, "+");

            while (token != NULL) {
                token = trim(token);
                size_t len = strlen(token);
                if (len >= 2 && token[0] == '"' && token[len - 1] == '"') {
                    token[len - 1] = '\0';
                    strcat(output, token + 1);
                } else {
                    char *var_value = get_variable(token);
                    if (var_value) {
                        strcat(output, var_value);
                    } else {
                        strcat(output, "(undefined)");
                    }
                }
                token = strtok(NULL, "+");
            }
            printf("%s\n", output);
        }
        // Handle 'kitten' command (variable declaration)
        else if (strncmp(trimmed_line, "kitten", 6) == 0) {
            char *rest = trim(trimmed_line + 6);
            char *equals = strchr(rest, '=');
            if (equals) {
                *equals = '\0';
                char *name = trim(rest);
                char *value = trim(equals + 1);

                // Remove quotes if present
                if (value[0] == '"' && value[strlen(value) - 1] == '"') {
                    value++;
                    value[strlen(value) - 1] = '\0';
                }
                set_variable(name, value);
                if (verbose) printf("Variable '%s' set to '%s'.\n", name, value);
            } else {
                fprintf(stderr, "Syntax error in variable declaration.\n");
            }
        }
        // Handle 'meow' command (user input)
        else if (strncmp(trimmed_line, "meow", 4) == 0) {
            char *var_name = trim(trimmed_line + 4);
            char prompt[INPUT_SIZE];
            snprintf(prompt, sizeof(prompt), "Enter value for %s: ", var_name);
            char *input = get_user_input(prompt);
            if (input == NULL || input[0] == '\0') {
                fprintf(stderr, "Error: Input for %s is empty.\n", var_name);
                continue;
            }
            set_variable(var_name, input);
            if (verbose) printf("Variable '%s' updated with value '%s'.\n", var_name, input);
        }
        // Handle 'neko_window' command (create window)
        else if (strncmp(trimmed_line, "neko_window", 11) == 0) {
            local_opengl_mode = 1; // Enable OpenGL mode
            char *args = trim(trimmed_line + 11);
            char *title = strtok(args, ",");
            char *width_str = strtok(NULL, ",");
            char *height_str = strtok(NULL, ",");

            if (title && width_str && height_str) {
                // Remove quotes if present
                if (title[0] == '"' && title[strlen(title) - 1] == '"') {
                    title[strlen(title) - 1] = '\0';
                    title++;
                }
                int width = atoi(width_str);
                int height = atoi(height_str);
                neko_window(title, width, height);
                if (verbose) printf("OpenGL window '%s' created with size %dx%d.\n", title, width, height);
            } else {
                fprintf(stderr, "Invalid arguments for 'neko_window'.\n");
            }
        }
        // Handle 'neko_draw_scene' command (draw scene)
        else if (strcmp(trimmed_line, "neko_draw_scene") == 0) {
            if (local_opengl_mode) {
                neko_draw_scene();
                glfwSwapBuffers(gl_window);
                glfwPollEvents();

                // Handle window close event
                if (glfwWindowShouldClose(gl_window)) {
                    break;
                }
            } else {
                fprintf(stderr, "Error: OpenGL not initialized. Use 'neko_window' first.\n");
            }
        }
        // Handle 'neko_add_block' command (add block)
        else if (strncmp(trimmed_line, "neko_add_block", 14) == 0) {
            float x, y, z;
            // Parse parameters: x, y, z
            if (sscanf(trimmed_line, "neko_add_block %f, %f, %f", &x, &y, &z) == 3) {
                neko_add_block(x, y, z);
            } else {
                fprintf(stderr, "Invalid arguments for 'neko_add_block'.\n");
            }
        }
        // Handle 'neko_remove_block' command (remove block)
        else if (strncmp(trimmed_line, "neko_remove_block", 17) == 0) {
            float x, y, z;
            // Parse parameters: x, y, z
            if (sscanf(trimmed_line, "neko_remove_block %f, %f, %f", &x, &y, &z) == 3) {
                neko_remove_block(x, y, z);
            } else {
                fprintf(stderr, "Invalid arguments for 'neko_remove_block'.\n");
            }
        }
        // Handle 'neko_func' command (function definition)
        else if (strncmp(trimmed_line, "neko_func", 9) == 0) {
            char *rest = trim(trimmed_line + 9);
            char *equals = strchr(rest, '=');
            if (equals) {
                *equals = '\0';
                char *func_name = trim(rest);
                char *func_code = trim(equals + 1);
                store_function(func_name, func_code);
                if (verbose) printf("Function '%s' stored.\n", func_name);
            } else {
                fprintf(stderr, "Syntax error in function definition.\n");
            }
        }
        // Handle 'call_func' command (function call)
        else if (strncmp(trimmed_line, "call_func", 9) == 0) {
            char *func_name = trim(trimmed_line + 9);
            char *func_code = get_function_code(func_name);
            if (func_code) {
                if (verbose) printf("Calling function '%s'.\n", func_name);
                interpret(func_code, local_opengl_mode); // Recursive call
            } else {
                fprintf(stderr, "Error: Function '%s' not defined.\n", func_name);
            }
        }
        // Handle 'is_key_pressed' command
        else if (strncmp(trimmed_line, "is_key_pressed", 14) == 0) {
            char key[NAME_SIZE];
            if (sscanf(trimmed_line, "is_key_pressed \"%[^\"]\"", key) == 1) {
                int pressed = is_key_pressed(key);
                printf("%d\n", pressed);
            } else {
                fprintf(stderr, "Invalid arguments for 'is_key_pressed'.\n");
            }
        }
        // Handle 'neko_set_player_position' command
        else if (strncmp(trimmed_line, "neko_set_player_position", 23) == 0) {
            float x, y, z;
            if (sscanf(trimmed_line, "neko_set_player_position %f, %f, %f", &x, &y, &z) == 3) {
                neko_set_player_position(x, y, z);
            } else {
                fprintf(stderr, "Invalid arguments for 'neko_set_player_position'.\n");
            }
        }
        // Handle unknown commands
        else {
            fprintf(stderr, "Unknown command: %s\n", trimmed_line);
        }
    }

    // If GUI mode is enabled, run the main OpenGL loop
    if (local_opengl_mode) {
        // Main loop
        while (!glfwWindowShouldClose(gl_window)) {
            neko_draw_scene();
            glfwSwapBuffers(gl_window);
            glfwPollEvents();

            // Basic player movement controls
            if (glfwGetKey(gl_window, GLFW_KEY_W) == GLFW_PRESS) {
                player_z -= 0.05f;
            }
            if (glfwGetKey(gl_window, GLFW_KEY_S) == GLFW_PRESS) {
                player_z += 0.05f;
            }
            if (glfwGetKey(gl_window, GLFW_KEY_A) == GLFW_PRESS) {
                player_x -= 0.05f;
            }
            if (glfwGetKey(gl_window, GLFW_KEY_D) == GLFW_PRESS) {
                player_x += 0.05f;
            }

            // Close window on ESC key
            if (glfwGetKey(gl_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(gl_window, GLFW_TRUE);
            }
        }

        // Clean up resources after the loop
        cleanup();
    }
}

// Function to read code from a file
char* read_code_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open file %s\n", filename);
        return NULL;
    }

    // Determine file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Allocate memory for code
    char *code = (char *)malloc(file_size + 1);
    if (code == NULL) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        fclose(file);
        return NULL;
    }

    // Read file content
    size_t read_size = fread(code, 1, file_size, file);
    code[read_size] = '\0'; // Null-terminate
    fclose(file);

    return code;
}

// Function to detect if the script uses GUI mode
int detect_gui_mode(const char *code) {
    // Look for the presence of 'neko_window' command
    if (strstr(code, "neko_window") != NULL) {
        return 1; // GUI mode detected
    }
    return 0; // Console mode
}

// Function to clean up resources
void cleanup() {
    // Delete OpenGL resources if initialized
    if (opengl_initialized) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
    }

    // Destroy GLFW window if created
    if (gl_window) {
        glfwDestroyWindow(gl_window);
    }

    // Terminate GLFW if initialized
    if (opengl_initialized) {
        glfwTerminate();
    }

    // Free voxels
    Voxel *current = voxels_head;
    while (current != NULL) {
        Voxel *temp = current;
        current = current->next;
        free(temp);
    }

    // Free variables
    Variable *var = variables_head;
    while (var != NULL) {
        Variable *temp = var;
        var = var->next;
        free(temp);
    }

    // Free functions
    Function *func = functions_head;
    while (func != NULL) {
        Function *temp = func;
        func = func->next;
        free(temp->code);
        free(temp);
    }
}

// Alternate main for standalone interpreter
#ifdef BUILD_NEKO_INTERPRETER
int main(int argc, char *argv[]) {
    // Read code from standard input
    char code_buffer[FUNCTION_CODE_SIZE];
    size_t len = fread(code_buffer, 1, sizeof(code_buffer) - 1, stdin);
    code_buffer[len] = '\0'; // Null-terminate

    // Detect execution mode
    int gui_mode = detect_gui_mode(code_buffer);

    // Interpret the code
    interpret(code_buffer, gui_mode);

    return 0;
}
#endif // BUILD_NEKO_INTERPRETER

// Main function
int main(int argc, char *argv[]) {
    // Check for at least one argument
    if (argc < 2) {
        printf("Usage: %s <filename> [options]\n", argv[0]);
        printf("Options:\n");
        printf("  -v      Enable verbose mode for debugging\n");
        return EXIT_FAILURE;
    }

    // Parse command-line arguments for verbose mode
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        }
    }

    // Read the NekoLang script from the file
    char *code = read_code_from_file(argv[1]);
    if (code == NULL) {
        return EXIT_FAILURE;
    }

    // Detect execution mode (Console or GUI)
    int gui_mode = detect_gui_mode(code);

    if (verbose) {
        printf("Mode %s activated.\n", gui_mode ? "GUI" : "Console");
        printf("Executing script: %s\n", argv[1]);
    }

    // Interpret the script with the detected mode
    interpret(code, gui_mode);

    // Free the allocated memory for code
    free(code);

    // Clean up resources if not in GUI mode
    if (!gui_mode) {
        cleanup();
    }

    return EXIT_SUCCESS;
}
