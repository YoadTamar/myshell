// DONE: 2, 7, 8, 6

#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#define CMD_SIZE 1024
#define ARGS_SIZE 10
#define QUIT "quit"
#define PROMPT_MSG "hello"
#define ENTER_PROMPT_MSG "prompt"
#define EQUALL_MSG "="
#define DO_LAST_CMD_MSG "!!"
#define AMPER_MSG "&"

char *prompt = NULL;
char *last_cmd = NULL;

int amper = 0;