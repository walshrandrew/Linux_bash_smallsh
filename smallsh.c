#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <unistd.h> 

#define INPUT_LENGTH 2048
#define MAX_ARGS 512

int status = 0;

struct command_line
{
    char *argv[MAX_ARGS + 1];
    int argc;
    char *input_file;
    char *output_file;
    bool is_bg;
};

struct command_line *parse_input()
{
    char input[INPUT_LENGTH];
    struct command_line *curr_command = (struct command_line *) calloc(1, sizeof(struct command_line));

    // Get Input
    printf(": ");
    fflush(stdout);
    fgets(input, INPUT_LENGTH, stdin);


    // Tokenize input
    char *token = strtok(input, " \n");
    while(token) {
        if(!strcmp(token, "<")){
            curr_command->input_file = strdup(strtok(NULL, " \n"));
        } else if (!strcmp(token, ">")){
            curr_command->output_file = strdup(strtok(NULL, " \n"));
        } else if (!strcmp(token, "&")){
            curr_command->is_bg = true;
        } else {
            curr_command->argv[curr_command->argc++] = strdup(token);
        }
        token=strtok(NULL, " \n");
    }
    return curr_command;
}


void built_in_exit(){
    // kill background process if need be when exiting
    // Maybe free mem too? prob not. if need be you will need to have curr_command as a function input
    exit(0);
}


void built_in_cd(struct command_line *curr_command){
    //char cwd[MAX_ARGS];
    //char *curr_dir = getcwd(cwd, sizeof(cwd));
    //printf("Directory before function if statememnts: %s\n", curr_dir);

    if(curr_command->argc > 1){
        chdir(curr_command->argv[1]);
        //FOR DEBUGGING: char *curr_dir = getcwd(cwd, sizeof(cwd));
        //FOR DEBUGGING: printf("Directory after argv[1]: %s", curr_dir);

    } else if (curr_command->argc == 1){
        chdir(getenv("HOME"));
        //char *curr_dir = getcwd(cwd, sizeof(cwd));
        //printf("Directory after argv[1]: %s", curr_dir);
    } else {
        printf("build_in_cd failed\n");
        return;
    }

    //printf("%s", curr_dir);
}


void built_in_status(){
    printf("Exit value: %d\n", status);
}


int main()
{
    struct command_line *curr_command;
    while(true)
    {
        curr_command = parse_input();

        if (strcmp(curr_command->argv[0], "exit") == 0){
            built_in_exit();
        } else if (strcmp(curr_command->argv[0], "cd") == 0){
            built_in_cd(curr_command);
        } else if (strcmp(curr_command->argv[0], "status") == 0){
            built_in_status(curr_command);
        } //else {
            // calls external commands
        //}  

    }
    return EXIT_SUCCESS;
}
