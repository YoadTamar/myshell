#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <wait.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define INIT_PROMPT_MSG "hello"
#define QUIT_MSG ("quit")
#define LAST_CMD_MSG ("!!")
#define SPACE_MSG (" ")
#define CD_MSG ("cd")
#define ECHO_MSG ("echo")
#define DOLLAR_SIGN ('$')
#define ENTER_PROMPT_MSG ("prompt")
#define EQUALL_MSG ("=")
#define TILDA_SIGN ('~')
#define QUESTION_SIGN ('?')
#define AMPER_MSG ("&")
#define IF_MSG ("if")
#define FI_MSG ("fi")
#define THEN_MSG ("then")
#define ELSE_MSG ("else")

#define FILE_PERMISSIONS (0644)
#define CMD_LENGTH (1024)
#define ARGS_LENGTH (10)

char *prompt = NULL;
char *last_cmd = NULL;

typedef struct var *pvar;
typedef struct var
{
    char var_name[20];
    char var_value[1024];
} var, *pvar;

pvar *var_array = NULL;
int var_array_len = 0;

int status = 0;
int amper = 0;


void
free_shell()
{
    free(prompt);
    prompt = NULL;

    free(last_cmd);
    last_cmd = NULL;

    for(int i = 0; i < var_array_len; i++)
    {
        free(var_array[i]);
        var_array[i] = NULL; 
    }
    free(var_array);
    var_array = NULL;
}

void
terminate_signal_handler(int sig)
{
    (void)sig;
    fprintf(stdout, "\nYou typed Control-C!\n");
    return;
}

void
cp_dynamic_str(char **dst, char *src)
{
    char *new_dst = NULL;
    new_dst = realloc(*dst, (strlen(src) + 1) * sizeof(char));
    if (!new_dst)
    {
        free(*dst);
        *dst = NULL;

        free_shell();

        perror("Failed to re-allocate new dest");
        exit(errno);
    }

    if(!strcpy(new_dst, src))
    {
        free(*dst);
        *dst = NULL;

        free_shell();

        perror("Cannot copy new message to dest");
        exit(errno);
    }
    
    *dst = new_dst;

    return;
}

int
split_cmd(char *cmd, char **args)
{
    int i = 0;
    char *token = NULL;

    token = strtok(cmd, SPACE_MSG);
    while(token != NULL)
    {
        args[i] = token;
        token = strtok(NULL, SPACE_MSG);
        i++;
    }
    args[i] = NULL;

    return i;
}

void
reset_args(char **args, int args_len)
{
    int i = 0;
    for(; i < args_len; i++)
    {
        args[i] = NULL;
    }
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
            strncpy(var_array[i]->var_value, args[2], sizeof(var_array[i]->var_value) - 1);
            var_array[i]->var_value[sizeof(var_array[i]->var_value) - 1] = '\0';
            return;
        }
    }

    var_array_len += 1;
    var_array = (pvar*)(realloc(var_array, (sizeof(pvar) * (var_array_len))));
    if(!var_array)
    {
        free_shell();
        perror("Cannot re-allocate new enviromet array");
        exit(errno);
    }

    pvar new_var = NULL;
    new_var = (pvar)(calloc(1, sizeof(var)));
    if(!new_var)
    {
        free_shell();
        perror("Cannot create new enviromet variable");
        exit(errno);
    }

    strncpy(new_var->var_name, (args[0] + 1), sizeof(new_var->var_name) - 1);
    new_var->var_name[sizeof(new_var->var_name) - 1] = '\0';

    strncpy(new_var->var_value, args[2], sizeof(new_var->var_value) - 1);
    new_var->var_value[sizeof(new_var->var_value) - 1] = '\0';

    var_array[var_array_len - 1] = new_var;
    return;
}

void
change_dir(char **args)
{
    char *moveto = NULL;

    if (args[1][0] == TILDA_SIGN)
    {
        const char *homeDir = getenv("HOME");
        if (!homeDir)
        {
            perror("Failed to get HOME environment variable");
            exit(errno);
        }

        size_t movetoSize = 0;
        movetoSize = strlen(homeDir) + strlen(args[1]) + 1;
        moveto = calloc(movetoSize, sizeof(char));
        if (!moveto)
        {
            free_shell();
            perror("Failed to allocate dest directory");
            exit(errno);
        }

        strcpy(moveto, homeDir);
        strcat(moveto, args[1] + 1);

        if (chdir(moveto) != 0)
        {
            free(moveto);
            moveto = NULL;

            free_shell();

            perror("Failed to move directory");
            exit(errno);
        }

        free(moveto);
        moveto = NULL;
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("Failed to move directory");
            exit(errno);
        }
    }
    return;
}

void
handle_echo(char *arg)
{
    if((strlen(arg) <= 2) && (arg[1] == QUESTION_SIGN)) // echo $?
    {
        fprintf(stdout, "%d\n", status); // NOT WORKS
    }
    else
    {
        int i = 0;
        const char *var_name = arg + 1;
        for(; i < var_array_len; i++)
        {
            if(!strcmp(var_array[i]->var_name, var_name))
            {
                fprintf(stdout, "%s\n", var_array[i]->var_value);
                break;
            }
        }
    }
    return;
}

void
execute_cmd(char **args) {
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
                    execute_cmd(&args[i]); // Now execute the remaining commands
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
execute_query(char **args, int args_len)
{
    if(args_len == 2)
    {
        if(!strcmp(args[0], CD_MSG))
        {
            change_dir(args);
            return;
        }
        else if((!strcmp(args[0], ECHO_MSG)) && (args[1][0] == DOLLAR_SIGN))
        {
            handle_echo(args[1]);
            return;
        }
    }
    else if(args_len == 3)
    {
        if((!strcmp(args[0], ENTER_PROMPT_MSG)) && (!strcmp(args[1], EQUALL_MSG)))
        {
            cp_dynamic_str(&prompt, args[2]);
            return;
        }
        else if((strlen(args[0]) > 1) && (args[0][0] == DOLLAR_SIGN) && (!strcmp(args[1], EQUALL_MSG)))
        {
            create_env_variable(args);
            return;
        }
    }

    if(!strcmp(args[0], IF_MSG))
    {
        
    }


    execute_cmd(args);
}


void
split_if(char *cmd, char **args)
{
    int i = 0;
    char *token = NULL;

    token = strtok(cmd, SPACE_MSG);
    while(token != NULL)
    {
        args[i] = token;
        token = strtok(NULL, SPACE_MSG);
        i++;
    }
    args[i] = NULL;
}


void
handle_if(char *cmd)
{
    
}



int
main(int argc, char **argv)
{
    signal(SIGINT, terminate_signal_handler);

    char cmd[CMD_LENGTH] = {'\0'}       ,
        *args[ARGS_LENGTH] = {'\0'}     ,
        *args_last[ARGS_LENGTH] = {'\0'};
    
    int args_len = 0     ,
        args_last_len = 0;


    prompt = (char*)(calloc((strlen(INIT_PROMPT_MSG) + 1), sizeof(char)));
    if(!prompt)
    {
        free_shell();
        perror("Cannot init prompt");
        exit(errno);
    }
    if(!strncpy(prompt, INIT_PROMPT_MSG, (strlen(INIT_PROMPT_MSG) + 1)))
    {
        free_shell();
        perror("Cannot copy init message to prompt");
        exit(errno);
    }

    last_cmd = (char*)(calloc(1, sizeof(char)));
    if(!last_cmd)
    {
        free_shell();
        perror("Cannot init last_cmd");
        exit(errno);
    }

    var_array = (pvar*)(calloc(1, sizeof(var)));
    if(!var_array)
    {
        free_shell();
        perror("Cannot init enviroment array");
        exit(errno);
    }

    while(1)
    {
        memset(cmd, 0, CMD_LENGTH);
        reset_args(args, ARGS_LENGTH);
        args_len = 0;
        amper = 0;

        fprintf(stdout, "%s: ", prompt);

        if(!fgets(cmd, CMD_LENGTH, stdin))
        {
            free_shell();
            perror("fgets failed to get input");
            exit(errno);
        }
        cmd[strlen(cmd) - 1] = '\0';
        args_len = split_cmd(cmd, args);

        if(args[0] == NULL)
        {
            continue;
        }
        else if(!strcmp(args[0], QUIT_MSG))
        {
            break;
        }
        
        if(!strcmp(args[args_len - 1], AMPER_MSG))
        {
            amper = 1;
            args[args_len - 1] = NULL;
            args_len -= 1;
        }

        if(!strcmp(args[0], LAST_CMD_MSG))
        {
            printf("!!\n");
            execute_query(args_last, args_last_len);
        }
        else
        {
            for(int i = 0; i < ARGS_LENGTH; i++)
            {
                args_last[i] = args[i];
            }
            args_last_len = args_len;

            execute_query(args, args_len);
        }
    }

    free_shell();

    return 0;
}