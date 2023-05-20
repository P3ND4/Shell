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

    // Itero por los tokens del comando
    for (i = 0; i < num_tokens; i++)
    {
        if (strcmp(tokens[i], ">") == 0)
        {
            // direccion de salida
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

            break;
        }
        else if (strcmp(tokens[i], ">>") == 0)
        {
            // redireccion de salida, agregando
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

            break;
        }
        else if (strcmp(tokens[i], "<") == 0)
        {
            // redireccion de entrada
            input_file = tokens[i + 1];
            input_fd = open(input_file, O_RDONLY);
            if (input_fd == -1)
            {
                perror("error al abrir archivo");
                exit(EXIT_FAILURE);
            }
            break;
        }
        else if (strcmp(tokens[i], "|") == 0)
        {
            // Pipe
            pipe_index = i;
            break;
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

        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fail");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            
            if (input_fd != STDIN_FILENO)
            {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);             
            }
            if (output_fd != STDOUT_FILENO)
            {
                dup2(output_fd, STDOUT_FILENO); 
                close(output_fd);               
            }

            if(strcmp(tokens[i],">") == 0)
            {
                char *newtoken[i];
                for (size_t j = 0; j < i; j++)
                {
                    newtoken[j] = tokens[j];
                }
                execvp(newtoken[0], newtoken);
                perror("error de comando");
                exit(EXIT_FAILURE); 

            }
            
            execvp(tokens[0], tokens);
            perror("error de comando");
            exit(EXIT_FAILURE);
        }
        
        waitpid(pid, NULL, 0); 
    }
}