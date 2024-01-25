#include "myshell.h"


pvar var_array[1000];
int var_array_len = 0;


void execute_terminal_cmd(char **args) {
    int pid, file;

    pid = fork();
    if (pid < 0) {
        perror("fork() failed");
        exit(errno);
    }
    if (pid == 0) {
        signal(SIGINT, SIG_DFL);

        for (int i = 0; args[i] != NULL; i++) {
            if (!strcmp(args[i], ">") || !strcmp(args[i], ">>")) {
                int flags = O_WRONLY | O_CREAT | (!strcmp(args[i], ">>") ? O_APPEND : O_TRUNC);
                file = open(args[i + 1], flags, FILE_PERMISSIONS);
                if (file < 0) {
                    perror("open() failed");
                    exit(errno);
                }
                dup2(file, STDOUT_FILENO);
                close(file);
                args[i] = NULL;
                break;
            } else if (!strcmp(args[i], "2>")) {
                file = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, FILE_PERMISSIONS);
                if (file < 0) {
                    perror("open() failed");
                    exit(errno);
                }
                dup2(file, STDERR_FILENO);
                close(file);
                args[i] = NULL;
                break;
            } else if (!strcmp(args[i], "<")) {
                file = open(args[i + 1], O_RDONLY);
                if (file < 0) {
                    perror("open() failed");
                    exit(errno);
                }
                dup2(file, STDIN_FILENO);
                close(file);
                args[i] = NULL;
                break;
            } else if (!strcmp(args[i], "|")) {
                int fds[2];
                if (pipe(fds) < 0) {
                    perror("pipe() failed");
                    exit(errno);
                }
                pid_t pid2 = fork();
                if (pid2 < 0) {
                    perror("fork() failed");
                    exit(errno);
                }
                if (pid2 == 0) { // Child process
                    // Connect the pipe, execute the command, then exit
                    dup2(fds[1], STDOUT_FILENO);
                    close(fds[0]);
                    close(fds[1]);
                    args[i] = NULL; // Terminate args array before the pipe symbol
                    execvp(args[0], args); // Execute the command before the pipe
                    perror("execvp() in the child process failed");
                    exit(errno);
                } else { // Parent process
                    // Shift remaining commands forward and recursively call execute_terminal_cmd
                    dup2(fds[0], STDIN_FILENO);
                    close(fds[0]);
                    close(fds[1]);
                    i++;
                    waitpid(pid2, NULL, 0); // Wait for the child process to finish
                    execute_terminal_cmd(&args[i]); // Now execute the remaining commands
                    exit(0); // Exit after executing the remaining commands
                }
            }
        }

        if (args[0] != NULL) {
            execvp(args[0], args); // No pipes remain, execute the final command
            perror("execvp() failed");
            exit(errno);
        }
    } else {
        waitpid(pid, NULL, 0); // Wait for the child process to finish
    }
}





void
signal_handler(int sig)
{
    (void)sig;
    fprintf(stdout, "\nYou typed Control-C!\n");
    return;
}


int
split_command(char* command, char **args)
{
    int i = 0;
    char *token = NULL;

    token = strtok(command, " ");
    while(token != NULL)
    {
        args[i] = token;
        token = strtok(NULL, " ");
        i++;
    }
    args[i] = NULL;

    return i;
}


void
reset_args(char **args, int size)
{
    int i = 0;
    for(; i < size; i++)
    {
        args[i] = NULL;
    }
    return;
}


void
cmp_str(char *prompt, const char* new_prompt)
{
    char *tmp_prompt = NULL;

    tmp_prompt = prompt;
    prompt = (char*)realloc(prompt, (strlen(new_prompt) + 1));
    if(prompt == NULL)
    {
        free(tmp_prompt);
        tmp_prompt = NULL;

        perror("realloc(3) failed");
        exit(errno);
    }
    strncpy(prompt, new_prompt, (strlen(new_prompt) + 1));

    return;
}


void
create_env_variable(char **args)
{
    int i = 0;
    for(; i < var_array_len; i++)
    {
        if(!strcmp(var_array[i]->var_name, (const char*)(args[0] + 1)))
        {
            break;
        }
    }
    if(i < var_array_len)
    {
        strncpy(var_array[i]->var_value, args[2], sizeof(var_array[i]->var_value) - 1);
        var_array[i]->var_value[sizeof(var_array[i]->var_value) - 1] = '\0';
    }
    else
    {
        pvar variable = NULL;
        variable = (pvar)calloc(1, sizeof(var));
        if(variable == NULL)
        {
            perror("calloc(3) failed [variable]");
            exit(errno);
        }

        strncpy(variable->var_name, (args[0] + 1), sizeof(variable->var_name) - 1);
        variable->var_name[sizeof(variable->var_name) - 1] = '\0';

        strncpy(variable->var_value, args[2], sizeof(variable->var_value) - 1);
        variable->var_value[sizeof(variable->var_value) - 1] = '\0';

        var_array_len += 1;

        if(var_array_len <= 1000)
        {
            var_array[var_array_len - 1] = variable;
        }
        else
        {
            fprintf(stderr, "Exceeded maximum number of variables\n");
            free(variable);
            variable = NULL;
            exit(EXIT_FAILURE);
        }
        
    }
}

void
change_dir(char **args)
{
    char *moveto = NULL;

    if(args[1][0] == '~')
    {
        moveto = (char*)calloc((strlen(getenv("HOME")) + 1) + (strlen(args[1] + 1) + 1), sizeof(char));
        if(moveto == NULL)
        {
            perror("calloc(3) failed [moveto]");
            exit(errno);
        }
        strncpy(moveto, getenv("HOME"), strlen(getenv("HOME")));
        strncpy(moveto + strlen(getenv("HOME")), args[1] + 1, strlen(args[1] + 1) + 1);
        printf("%s\n", moveto);
        if(chdir(moveto) != 0)
        {
            perror("chdir");
            exit(errno);
        }

        free(moveto);
        moveto = NULL;
    }
    else
    {
        if(chdir(args[0]) != 0)
        {
            perror("chdir");
            exit(errno);
        }
    }
}


void
handle_echo(char *arg)
{
    printf("here\n");
    int i = 0;
    if(strlen(arg) < 2 && arg[1] == '?')
    {
        char *std = {"echo", "$?", NULL};
        execute_terminal_cmd(std);
    }
    else
    {
        arg += 1;
        for(; i < var_array_len; i++)
        {
            if(!strcmp(var_array[i]->var_name, arg))
            {
                printf("%s\n", var_array[i]->var_value);
                break;
            }
        }
    }
    
}


void
execute_query(char **args, int args_len)
{
    if(args_len == 2)
    {
        if(!strcmp(args[0], "cd"))
        {
            change_dir(args);
        }
        if((!strcmp(args[0], "echo")) && (args[1][0] == '$'))
        {
            handle_echo(args[1]);
        }
    }
    else if(args_len == 3)
    {
        if((!strcmp(args[0], ENTER_PROMPT_MSG)) && (!strcmp(args[1], EQUALL_MSG)))
        {
            cmp_str(prompt, args[2]);
        }
        else if((strlen(args[0]) > 1) && args[0][0] == '$' && (!strcmp(args[1], EQUALL_MSG)))
        {
            create_env_variable(args);
        }
    }
    else
    {
        execute_terminal_cmd(args);
    }
}


int
main()
{
    signal(SIGINT, signal_handler);

    int args_len = 0, args_len_last = 0;

    char command[CMD_SIZE] = {0};
    char *args[ARGS_SIZE] = {0};
    char *args_last[ARGS_SIZE] = {0};

    prompt = (char*)calloc((strlen(PROMPT_MSG) + 1), sizeof(char));
    if(prompt == NULL)
    {
        perror("calloc(3) failed [prompt]");
        exit(errno);
    }
    strncpy(prompt, PROMPT_MSG, (strlen(PROMPT_MSG) + 1));

    last_cmd = (char*)calloc(1, sizeof(char));
    if(last_cmd == NULL)
    {
        perror("calloc(3) failed [last_cmd]");
        exit(errno);
    }

    while(1)
    {
        memset(command, 0, CMD_SIZE);
        reset_args(args, ARGS_SIZE);

        fprintf(stdout, "%s: ", prompt);

        fgets(command, CMD_SIZE, stdin);
        command[strlen(command) - 1] = '\0';
        
        args_len = split_command(command, args);
    
        if(args[0] == NULL)
        {
            continue;
        }

        else if(!strcmp(args[0], QUIT))
        {
            break;
        }

        if(!strcmp(args[args_len - 1], "&")) 
        {
            amper = 1;
            args[args_len - 1] = NULL;
            args_len -= 1;
        }

        else if(!strcmp(args[0], DO_LAST_CMD_MSG))
        {
            execute_query(args_last, args_len_last);
        }
        else
        {
            cmp_str(last_cmd, command);
            args_len_last = split_command(command, args_last);

            execute_query(args, args_len);
        }
    }

    free(prompt);
    prompt = NULL;

    for(int i = 0; i < var_array_len; i++)
    {
        free(var_array[i]);
        var_array[i] = NULL;
    }
    
    return 0;
}