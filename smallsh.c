#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <unistd.h> 

#define INPUT_LENGTH 2048
#define MAX_ARGS 512

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

void built_in_cd(struct command_line *curr_command);
//void built_in_status();


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
        } //else if (strcmp(curr_command->argv[0], "status") == 0){
            //built_in_status(curr_command);
        //} //else {
            // calls external commands
        //}  

    }
    return EXIT_SUCCESS;
}
