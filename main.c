#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>
#include "shell.h"

int maxtoken = 100;

int main()
{
    char input[1024];
    char *tokens[maxtoken];
    int num_tokens;
    FILE *file;
    char file_content[1024];

    while (1)
    {
        prompt();

        // Read input
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            continue;
        }

        // Remove newline character
        input[strcspn(input, "\n")] = '\0';

        // Ignore comments
        char *comment = strchr(input, '#');
        if (comment != NULL)
        {
            *comment = '\0';
        }

        // Parse tokens
        num_tokens = 0;
        char *token = strtok(input, " ");
        while (token != NULL)
        {
            tokens[num_tokens++] = token;
            token = strtok(NULL, " ");
        }
        tokens[num_tokens] = NULL;

        // Handle built-in commands
        if (strcmp(tokens[0], "cd") == 0)
        {

            if (num_tokens > 2 && strcmp(tokens[1], "<") == 0)
            {
                // Abrir el archivo en modo lectura
                file = fopen(tokens[2], "r");

                // Verificar si el archivo se abrió correctamente
                if (file == NULL)
                {
                    printf("No se pudo abrir el archivo\n");
                    exit(EXIT_FAILURE);
                }

                // Leer el archivo
                fscanf(file, "%s", file_content);

                // Cerrar el archivo
                fclose(file);

                // Cambiar el directorio actual a la ruta leída del archivo
                if (chdir(file_content) == -1)
                {
                    printf("No se pudo cambiar al directorio especificado.n");
                    exit(EXIT_FAILURE);
                }
            }

            else if (num_tokens > 2)
            {
                fprintf(stderr, "cd: too many arguments\n");
            }

            else if (num_tokens == 1)
            {
                chdir(getenv("HOME")); // Change to home directory
            }
            else
            {
                if (chdir(tokens[1]) == -1)
                {
                    perror("cd");
                }
            }
        }
        else if (strcmp(tokens[0], "exit") == 0)
        {
            break;
        }
        else
        {
            execute(tokens, num_tokens);
        }
    }

    return 0;
}
