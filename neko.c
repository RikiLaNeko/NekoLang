// neko.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Taille maximale des entrées utilisateur
#define INPUT_SIZE 100

// Taille maximale des noms de variables et fonctions
#define NAME_SIZE 50

// Taille maximale des valeurs de variables
#define VALUE_SIZE 100

// Taille maximale du code des fonctions
#define FUNCTION_CODE_SIZE 1000

// Définition de structures pour les variables et les fonctions

// Structure pour stocker une variable
typedef struct Variable {
    char name[NAME_SIZE];
    char value[VALUE_SIZE];
    struct Variable *next;
} Variable;

// Structure pour stocker une fonction
typedef struct Function {
    char name[NAME_SIZE];
    char *code;
    struct Function *next;
} Function;

// Pointeurs vers les listes chaînées de variables et fonctions
Variable *variables_head = NULL;
Function *functions_head = NULL;

// Variables globales pour OpenGL
unsigned int VAO = 0, VBO = 0;
GLFWwindow* gl_window = NULL;
GLuint shaderProgram = 0;

// Indicateur d'initialisation OpenGL
int opengl_initialized = 0;

// Mode verbose
int verbose = 0;

// Déclarations des fonctions
char* get_user_input(const char *prompt);
void set_variable(const char *name, const char *value);
char* get_variable(const char *name);
void store_function(const char *name, const char *code);
char* get_function_code(const char *name);
char* trim(char *str);
void interpret(const char *code, int gui_mode);
void neko_window(const char *title, int width, int height);
void neko_draw_triangle();
char* read_code_from_file(const char *filename);
void setup_opengl_objects();
GLuint compile_shader(const char* source, GLenum type);
GLuint create_shader_program(const char* vertexSource, const char* fragmentSource);
void cleanup();
int detect_gui_mode(const char *code);

// Fonction pour obtenir l'entrée utilisateur avec sécurité
char* get_user_input(const char *prompt) {
    static char input[INPUT_SIZE]; // Buffer pour l'entrée utilisateur
    printf("%s", prompt);          // Afficher l'invite à l'utilisateur
    if (fgets(input, sizeof(input), stdin) != NULL) { // Lire l'entrée
        input[strcspn(input, "\n")] = '\0'; // Supprimer le caractère de nouvelle ligne
    }
    return input;
}

// Fonction pour définir ou mettre à jour une variable
void set_variable(const char *name, const char *value) {
    Variable *current = variables_head;
    // Parcourir la liste pour vérifier si la variable existe déjà
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            strncpy(current->value, value, VALUE_SIZE - 1);
            current->value[VALUE_SIZE - 1] = '\0';
            return;
        }
        current = current->next;
    }
    // Si la variable n'existe pas, créer une nouvelle variable
    Variable *new_var = (Variable *)malloc(sizeof(Variable));
    if (new_var == NULL) {
        fprintf(stderr, "Erreur d'allocation mémoire pour la variable.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(new_var->name, name, NAME_SIZE - 1);
    new_var->name[NAME_SIZE - 1] = '\0';
    strncpy(new_var->value, value, VALUE_SIZE - 1);
    new_var->value[VALUE_SIZE - 1] = '\0';
    new_var->next = variables_head;
    variables_head = new_var;
}

// Fonction pour obtenir la valeur d'une variable
char* get_variable(const char *name) {
    Variable *current = variables_head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current->value;
        }
        current = current->next;
    }
    return NULL; // Variable non trouvée
}

// Fonction pour stocker une définition de fonction
void store_function(const char *name, const char *code) {
    Function *current = functions_head;
    // Vérifier si la fonction existe déjà
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            free(current->code); // Libérer l'ancien code
            current->code = strdup(code); // Copier le nouveau code
            return;
        }
        current = current->next;
    }
    // Si la fonction n'existe pas, créer une nouvelle fonction
    Function *new_func = (Function *)malloc(sizeof(Function));
    if (new_func == NULL) {
        fprintf(stderr, "Erreur d'allocation mémoire pour la fonction.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(new_func->name, name, NAME_SIZE - 1);
    new_func->name[NAME_SIZE - 1] = '\0';
    new_func->code = strdup(code);
    if (new_func->code == NULL) {
        fprintf(stderr, "Erreur d'allocation mémoire pour le code de la fonction.\n");
        free(new_func);
        exit(EXIT_FAILURE);
    }
    new_func->next = functions_head;
    functions_head = new_func;
}

// Fonction pour obtenir le code d'une fonction par son nom
char* get_function_code(const char *name) {
    Function *current = functions_head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current->code;
        }
        current = current->next;
    }
    return NULL; // Fonction non trouvée
}

// Fonction pour supprimer les espaces en début et en fin, et les points-virgules
char* trim(char *str) {
    if (str == NULL) return NULL; // Gérer les entrées NULL

    // Supprimer les espaces en début
    while (*str == ' ' || *str == '\t') str++;

    // Supprimer les espaces en fin
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\n' || *end == ';')) end--;
    *(end + 1) = '\0'; // Terminer la chaîne

    return str;
}

// Fonction pour compiler un shader
GLuint compile_shader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Vérifier les erreurs de compilation
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        const char *shader_type = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        fprintf(stderr, "Erreur de compilation du shader %s:\n%s\n", shader_type, infoLog);
    }
    return shader;
}

// Fonction pour créer un programme shader
GLuint create_shader_program(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compile_shader(vertexSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compile_shader(fragmentSource, GL_FRAGMENT_SHADER);

    // Créer le programme shader
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Vérifier les erreurs de lien
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "Erreur de lien du programme shader:\n%s\n", infoLog);
    }

    // Supprimer les shaders une fois liés
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// Fonction pour initialiser les objets OpenGL
void setup_opengl_objects() {
    // Définition des vertices du triangle
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,  // Bas à gauche
         0.5f, -0.5f, 0.0f,  // Bas à droite
         0.0f,  0.5f, 0.0f   // Haut
    };

    // Générer et lier le Vertex Array Object (VAO)
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    // Générer et lier le Vertex Buffer Object (VBO)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Spécifier le format des vertices
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Délier le VAO et le VBO pour éviter les modifications involontaires
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Définir les sources des shaders
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "void main() {\n"
        "   gl_Position = vec4(aPos, 1.0);\n"
        "}\0";

    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "   FragColor = vec4(1.0, 0.5, 0.2, 1.0);\n"
        "}\n\0";

    // Créer le programme shader
    shaderProgram = create_shader_program(vertexShaderSource, fragmentShaderSource);
    opengl_initialized = 1; // Indiquer que OpenGL a été initialisé
}

// Fonction pour dessiner un triangle
void neko_draw_triangle() {
    // Nettoyer l'écran avec une couleur de fond (bleu foncé)
    glClearColor(0.0f, 0.0f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Utiliser le programme shader
    glUseProgram(shaderProgram);

    // Lier le VAO et dessiner le triangle
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Délier le VAO
    glBindVertexArray(0);
}

// Fonction pour créer une fenêtre OpenGL (Commande "neko_window")
void neko_window(const char *title, int width, int height) {
    if (!glfwInit()) {
        fprintf(stderr, "Erreur lors de l'initialisation de GLFW\n");
        exit(EXIT_FAILURE);
    }

    // Configuration des paramètres de la fenêtre
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE); // Activer le double buffering

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Nécessaire pour Mac
#endif

    // Créer la fenêtre GLFW
    gl_window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!gl_window) {
        fprintf(stderr, "Erreur lors de la création de la fenêtre OpenGL\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // Définir le contexte OpenGL
    glfwMakeContextCurrent(gl_window);

    // Activer le V-Sync
    glfwSwapInterval(1);

    // Initialiser GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Erreur lors de l'initialisation de GLEW\n");
        glfwDestroyWindow(gl_window);
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // Définir la zone de rendu
    glViewport(0, 0, width, height);
}

// Fonction pour interpréter et exécuter le code NekoLang
void interpret(const char *code, int gui_mode) {
    char line[256];          // Buffer pour chaque ligne de code
    const char *ptr = code;  // Pointeur pour parcourir le code
    int in_neko_block = 0;   // Flag pour vérifier si on est dans un bloc 'neko { }'
    int local_opengl_mode = gui_mode; // Mode OpenGL local basé sur le scan initial

    while (*ptr != '\0') {
        // Lire une ligne du code
        const char *line_start = ptr;
        while (*ptr != '\n' && *ptr != '\0') {
            ptr++;
        }
        size_t len = ptr - line_start;
        if (len >= sizeof(line)) len = sizeof(line) - 1; // Limiter la longueur
        strncpy(line, line_start, len);
        line[len] = '\0';
        if (*ptr == '\n') ptr++; // Passer le caractère de nouvelle ligne

        // Supprimer les espaces inutiles
        char *trimmed_line = trim(line);

        // Ignorer les lignes vides et les commentaires
        if (strlen(trimmed_line) == 0 || strncmp(trimmed_line, "//", 2) == 0) continue;

        // Début du bloc 'neko {'
        if (strcmp(trimmed_line, "neko {") == 0 || strcmp(trimmed_line, "neko{") == 0) {
            in_neko_block = 1;
            if (verbose) printf("Entrée dans le bloc 'neko'.\n");
            continue;
        }

        // Fin du bloc '}'
        if (strcmp(trimmed_line, "}") == 0) {
            in_neko_block = 0;
            if (verbose) printf("Sortie du bloc 'neko'.\n");
            continue;
        }

        if (!in_neko_block) {
            continue;
        }

        // Traitement des commandes
        // 1. Commande 'purr' pour afficher du texte
        if (strncmp(trimmed_line, "purr", 4) == 0) {
            char *msg = trim(trimmed_line + 4);
            char output[1024] = "";
            char *token = strtok(msg, "+");

            while (token != NULL) {
                token = trim(token);
                size_t len = strlen(token);
                if (len >= 2 && token[0] == '"' && token[len-1] == '"') {
                    token[len-1] = '\0';
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
            printf("%s\n", output);
        }
        // 2. Commande 'kitten' pour déclarer une variable
        else if (strncmp(trimmed_line, "kitten", 6) == 0) {
            char *rest = trim((char *)(trimmed_line + 6));

            char *equals = strchr(rest, '=');
            if (equals) {
                *equals = '\0';
                char *name = trim(rest);
                char *value = trim(equals + 1);

                // Supprimer les guillemets si présents
                if (value[0] == '"' && value[strlen(value)-1] == '"') {
                    value++;
                    value[strlen(value)-1] = '\0';
                }
                set_variable(name, value);
                if (verbose) printf("Variable '%s' définie avec la valeur '%s'.\n", name, value);
            } else {
                fprintf(stderr, "Erreur de syntaxe dans la déclaration de variable.\n");
            }
        }
        // 3. Commande 'meow' pour obtenir une entrée utilisateur
        else if (strncmp(trimmed_line, "meow", 4) == 0) {
            char *var_name = trim((char *)(trimmed_line + 4));

            char prompt[INPUT_SIZE];
            snprintf(prompt, sizeof(prompt), "Entrez la valeur pour %s: ", var_name);

            char *input = get_user_input(prompt);
            if (input == NULL || input[0] == '\0') {
                fprintf(stderr, "Erreur: L'entrée pour %s est vide.\n", var_name);
                continue;
            }
            set_variable(var_name, input);
            if (verbose) printf("Variable '%s' mise à jour avec la valeur '%s'.\n", var_name, input);
        }
        // 4. Commande 'neko_window' pour créer une fenêtre OpenGL
        else if (strncmp(trimmed_line, "neko_window", 11) == 0) {
            local_opengl_mode = 1; // Activer le mode OpenGL

            char *args = trim((char *)(trimmed_line + 11));
            char *title = strtok(args, ",");
            char *width_str = strtok(NULL, ",");
            char *height_str = strtok(NULL, ",");

            if (title && width_str && height_str) {
                // Supprimer les guillemets du titre si présents
                if (title[0] == '"' && title[strlen(title)-1] == '"') {
                    title[strlen(title)-1] = '\0';
                    title++;
                }
                int width = atoi(width_str);
                int height = atoi(height_str);
                neko_window(title, width, height);
                if (verbose) printf("Fenêtre OpenGL créée: '%s' (%dx%d).\n", title, width, height);
            } else {
                fprintf(stderr, "Erreur: Arguments invalides pour 'neko_window'.\n");
            }
        }
        // 5. Commande 'neko_draw_triangle' pour dessiner un triangle
        else if (strcmp(trimmed_line, "neko_draw_triangle") == 0) {
            if (local_opengl_mode) {
                neko_draw_triangle();
                glfwSwapBuffers(gl_window);
                glfwPollEvents();
            } else {
                fprintf(stderr, "Erreur: OpenGL n'est pas initialisé. Utilisez 'neko_window' d'abord.\n");
            }
        }
        // 6. Commande 'neko_func' pour définir une fonction
        else if (strncmp(trimmed_line, "neko_func", 9) == 0) {
            char *rest = trim((char *)(trimmed_line + 9));
            char *equals = strchr(rest, '=');
            if (equals) {
                *equals = '\0';
                char *func_name = trim(rest);
                char *func_code = trim(equals + 1);
                store_function(func_name, func_code);
                if (verbose) printf("Fonction '%s' stockée.\n", func_name);
            } else {
                fprintf(stderr, "Erreur de syntaxe dans la définition de fonction.\n");
            }
        }
        // 7. Commande 'call_func' pour appeler une fonction
        else if (strncmp(trimmed_line, "call_func", 9) == 0) {
            char *func_name = trim((char *)(trimmed_line + 9));
            char *func_code = get_function_code(func_name);
            if (func_code) {
                if (verbose) printf("Appel de la fonction '%s'.\n", func_name);
                interpret(func_code, local_opengl_mode); // Appeler récursivement l'interpréteur avec le code de la fonction
            } else {
                fprintf(stderr, "Erreur: Fonction '%s' non définie.\n", func_name);
            }
        }
        // Commande inconnue
        else {
            fprintf(stderr, "Commande inconnue: %s\n", trimmed_line);
        }
    }

    // Si le mode OpenGL est activé, exécuter la boucle principale OpenGL
    if (local_opengl_mode) {
        setup_opengl_objects();
        while (!glfwWindowShouldClose(gl_window)) {
            neko_draw_triangle();
            glfwSwapBuffers(gl_window);
            glfwPollEvents();
        }
        cleanup();
    }
}

// Fonction pour lire le code depuis un fichier
char* read_code_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir le fichier %s\n", filename);
        return NULL;
    }

    // Aller à la fin du fichier pour déterminer sa taille
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Allouer de la mémoire pour le code
    char *code = (char *)malloc(file_size + 1);
    if (code == NULL) {
        fprintf(stderr, "Erreur: Échec de l'allocation mémoire.\n");
        fclose(file);
        return NULL;
    }

    // Lire le contenu du fichier
    size_t read_size = fread(code, 1, file_size, file);
    code[read_size] = '\0'; // Terminer la chaîne
    fclose(file);

    return code;
}

// Fonction pour détecter si le script utilise OpenGL
int detect_gui_mode(const char *code) {
    // Rechercher la présence de 'neko_window' dans le code
    if (strstr(code, "neko_window") != NULL) {
        return 1; // Mode GUI détecté
    }
    return 0; // Mode console
}

// Fonction de nettoyage des ressources OpenGL et des listes chaînées
void cleanup() {
    // Supprimer les objets OpenGL seulement s'ils ont été initialisés
    if (opengl_initialized) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
    }

    // Détruire la fenêtre GLFW seulement si elle a été créée
    if (gl_window) {
        glfwDestroyWindow(gl_window);
    }

    // Terminer GLFW seulement si OpenGL a été initialisé
    if (opengl_initialized) {
        glfwTerminate();
    }

    // Libérer les variables
    Variable *var = variables_head;
    while (var != NULL) {
        Variable *temp = var;
        var = var->next;
        free(temp);
    }

    // Libérer les fonctions
    Function *func = functions_head;
    while (func != NULL) {
        Function *temp = func;
        func = func->next;
        free(temp->code);
        free(temp);
    }
}

#ifdef BUILD_NEKO_INTERPRETER
// Fonction main alternative pour l'interpréteur autonome
int main(int argc, char *argv[]) {
    // Lire le code depuis l'entrée standard
    char code_buffer[10000];
    size_t len = fread(code_buffer, 1, sizeof(code_buffer) - 1, stdin);
    code_buffer[len] = '\0'; // Terminer la chaîne

    // Détecter le mode d'exécution (console ou GUI)
    int gui_mode = detect_gui_mode(code_buffer);

    // Interpréter le code avec le mode détecté
    interpret(code_buffer, gui_mode);

    return 0;
}
#endif  // BUILD_NEKO_INTERPRETER

// Fonction principale
int main(int argc, char *argv[]) {
    // Vérifier les arguments
    if (argc < 2) {
        printf("Usage: %s <filename> [options]\n", argv[0]);
        printf("Options:\n");
        printf("  -v      Mode verbose (pour le débogage)\n");
        return EXIT_FAILURE;
    }

    // Vérifier les arguments pour le mode verbose
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        }
    }

    // Lire le code depuis le fichier
    char *code = read_code_from_file(argv[1]);
    if (code == NULL) {
        return EXIT_FAILURE;
    }

    // Détecter le mode d'exécution (console ou GUI)
    int gui_mode = detect_gui_mode(code);

    if (verbose) {
        printf("Mode %s activé.\n", gui_mode ? "GUI" : "Console");
        printf("Exécution du script: %s\n", argv[1]);
    }

    // Interpréter le code avec le mode détecté
    interpret(code, gui_mode);

    // Libérer la mémoire allouée pour le code
    free(code);

    // Nettoyer les ressources si non en mode GUI
    if (!gui_mode) {
        cleanup();
    }

    return EXIT_SUCCESS;
}
