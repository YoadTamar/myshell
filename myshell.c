#include "myshell.h"

void
signal_handler(int sig)
{
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
execute_query(char **args, int args_len)
{
    if((!strcmp(args[0], ENTER_PROMPT_MSG)) && (!strcmp(args[1], EQUALL_MSG)))
    {            
        cmp_str(prompt, args[2]);
    }
}


int
main(int argc, char **argv)
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

        if(feof(stdin))
        {
            fprintf(stdout, "Control-D\n");
            continue;
        }
        
        args_len = split_command(command, args);
    
        if(args[0] == NULL)
        {
            continue;
        }

        else if(!strcmp(args[0], QUIT))
        {
            break;
        }

        if(!strcmp(argv[args_len - 1], "&")) 
        {
            amper = 1;
            argv[args_len - 1] = NULL;
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
    
    return 0;
}