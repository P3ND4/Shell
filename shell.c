#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>
#include "shell.h"


void DirList()
{
    DIR *dir;
    struct dirent *ent;
    dir = opendir(".");
    while ((ent = readdir(dir)) != NULL)
    {
        printf("%s ", ent->d_name);
    }
    closedir(dir);
    printf("\n");
}

void prompt()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("%s MyShell-$ ", cwd);
}

void execute(char **tokens, int num_tokens)
{
    int i;
    int input_fd = STDIN_FILENO;
    int output_fd = STDOUT_FILENO;
    char *input_file = NULL;
    char *output_file = NULL;
    int append_output = 0;
    int pipe_index = -1;
    int redicIn = -1;
    int redicOut = -1;

    // Itero por los tokens del comando
    for (i = 0; i < num_tokens; i++)
    {
        if (strcmp(tokens[i], ">") == 0)
        {
            // direccion de salida
            redicOut = i;
            output_file = tokens[i + 1];
            output_fd = open(output_file, O_WRONLY | O_CREAT | (append_output ? O_APPEND : O_TRUNC), 0666);
            if (output_fd == -1)
            {
                perror("error al abrir archivo");
                exit(EXIT_FAILURE);
            }

            if (strcmp(tokens[i - 1], "dir") == 0)
            {
                // Redireccionar la salida estándar al archivo
                dup2(output_fd, STDOUT_FILENO);

                // Ejecutar el comando "dir"
                DirList();

                // Cerrar el archivo
                close(output_fd);

                // Redireccionar la salida estándar a la consola
                freopen("/dev/tty", "w", stdout);
            }
        }
        else if (strcmp(tokens[i], ">>") == 0)
        {
            // redireccion de salida, agregando
            redicOut = i;
            output_file = tokens[i + 1];
            append_output = 1;
            output_fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
            if (output_fd == -1)
            {
                perror("error al abrir archivo");
                exit(EXIT_FAILURE);
            }

            if (strcmp(tokens[i - 1], "dir") == 0)
            {
                // Redireccionar la salida estándar al archivo
                dup2(output_fd, STDOUT_FILENO);

                // Ejecutar el comando "dir"
                DirList();

                // Cerrar el archivo
                close(output_fd);

                // Redireccionar la salida estándar a la consola
                freopen("/dev/tty", "w", stdout);
            }
        }
        else if (strcmp(tokens[i], "<") == 0)
        {
            // redireccion de entrada
            redicIn = i;
            input_file = tokens[i + 1];
            input_fd = open(input_file, O_RDONLY);
            if (input_fd == -1)
            {
                perror("error al abrir archivo");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(tokens[i], "|") == 0)
        {
            pipe_index = i;
        }
    }

    if (pipe_index != -1)
    {

        int pipefd[2];
        if (pipe(pipefd) == -1)
        {
            perror("fail");
            exit(EXIT_FAILURE);
        }

        pid_t pid1 = fork();
        if (pid1 == -1)
        {
            perror("fail");
            exit(EXIT_FAILURE);
        }
        else if (pid1 == 0)
        {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);

            if (input_fd != STDIN_FILENO)
            {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
                tokens[redicIn] = NULL;
            }

            tokens[pipe_index] = NULL;
            execvp(tokens[0], tokens);
            perror("error de comando");
            exit(EXIT_FAILURE);
        }

        pid_t pid2 = fork();
        if (pid2 == -1)
        {
            perror("fail");
            exit(EXIT_FAILURE);
        }
        else if (pid2 == 0)
        {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);

            if (output_fd != STDOUT_FILENO)
            {
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);
                tokens[redicOut] = NULL;
            }

            execvp(tokens[pipe_index + 1], &tokens[pipe_index + 1]);
            perror("error de comando");
            exit(EXIT_FAILURE);
        }

        close(pipefd[0]);
        close(pipefd[1]);
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
    }
    else
    {
        // Create child process
        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fail");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            // Child process
            if (input_fd != STDIN_FILENO)
            {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
                tokens[redicIn] = NULL;
            }
            if (output_fd != STDOUT_FILENO)
            {
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);
                tokens[redicOut] = NULL;
            }

            // Execute command
            execvp(tokens[0], tokens);
            perror("error de comando");
            exit(EXIT_FAILURE);
        }

        waitpid(pid, NULL, 0);
    }
}
void UpdateHistorial(char *name, char *command)
{
    FILE *output = fopen(name, "a");

    if (output == NULL)
    {
        perror("No se pudo abrir el archivo de texto");
        exit(EXIT_FAILURE);
    }

    fprintf(output, "%s\n", command);
    fclose(output);
}

int LenHistorial(char *name)
{
    char line[1000];
    int contador = 0;
    FILE *fp = fopen(name, "r");

    while (fgets(line, sizeof(line), fp))
        contador++;

    fclose(fp);
    return contador;
}

char *PrintHistorialorAgain(int falg, int again)
{
    char history[] = "history.hst";
    FILE *fp = fopen(history, "r");
    int len_historial = LenHistorial(history);
    char line[1024];
    int n = len_historial - 10;
    if (again > len_historial)
        again = 10;

    if (len_historial <= 10)
    {
        int i = 1;
        while (fgets(line, sizeof(line), fp))
        {
            if (falg && i == again)
            {
                char *aginline = malloc(sizeof(char) * 1024);
                strcpy(aginline, line);
                return aginline;
            }
            if(!falg) printf("%i %s", i, line);
            i++;
        }

        return NULL;
    }

    fseek(fp, 0, SEEK_SET);

    for (int i = 0; i < n; i++)
    {
        if (fgets(line, sizeof(line), fp) == NULL)
        {
            printf("La línea deseada no existe.n");
            return NULL;
        }
    }
    int i = 1;
    while (fgets(line, sizeof(line), fp))
    {
        if (falg && i == again)
        {
            char *aginline = malloc(sizeof(char) * 1024);
            strcpy(aginline, line);
            return aginline;
        }
        if(!falg) printf("%i %s", i, line);

        i++;
    }

    fclose(fp);
    return NULL;
}
void exeHelp(char* func)
{
    char help[] = "/help";
    char slash[] = "/";
    char * newfunc = malloc(sizeof(char)*strlen(func));
    strcpy(newfunc,func);
    char path[1000];
    getcwd(path, 1000);
    char *hlpath = malloc(sizeof(char)*(strlen(path)+strlen(help)+1+strlen(newfunc)));
    hlpath = strcat(hlpath, path);
    
    hlpath = strcat(hlpath, help);
    
    hlpath = strcat(hlpath, slash);
    
    hlpath = strcat(hlpath, newfunc);
    
    hlpath = strcat(hlpath, ".hlp");
    int file = open(hlpath,O_RDONLY | O_CREAT,0600);
    if(file == -1){
        printf("La funcionalidad %s no esta implementada en esta shell \n", func);
        return;
    } 
    char text[2000];
    read(file, text, 2000);
    close(file);
    printf("\n");
    printf("%s\n",text);
    printf("\n");

}