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


void free_command(struct command_line *curr_command){
    if (curr_command){
        if (curr_command->input_file) free(curr_command->input_file);
        if (curr_command->output_file) free(curr_command->output_file);
        for (int i = 0; i < curr_command->argc; i++) free(curr_command->argv[i]);
        free(curr_command);
    }

}


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
    printf("exit value: %d\n", status);
}


void other_commands(struct command_line *curr_command){
    pid_t p = fork();
    if(p<0){
        perror("Failed Fork");
        exit(1);
    } else if (p == 0){ //child processes
        if(execvp(curr_command->argv[0], curr_command->argv) == -1){   // if execvp fails
            perror("child command failed");
            status = 1;                                                // update status
            exit(1);                                                   // terminate child
        }
    }else {    // parent process
        if (curr_command->is_bg){
            return ;
        } else { // wait for foreground child to finish
            int child;
            waitpid(p, &child, 0);

            if (WIFEXITED(child)){
                status = WEXITSTATUS(child);                        // storing exit status
            } else if (WIFSIGNALED(child)){
                status = WTERMSIG(child);                           // storing exit signal
            }
        }
    }    
}





int main()
{
    struct command_line *curr_command;
    while(true)
    {
        curr_command = parse_input();

        if (curr_command->argc == 0) {
            free_command(curr_command);
            continue;
        }

        if (strcmp(curr_command->argv[0], "exit") == 0){
            free_command(curr_command);
            built_in_exit();
        } else if (strcmp(curr_command->argv[0], "cd") == 0){
            built_in_cd(curr_command);
        } else if (strcmp(curr_command->argv[0], "status") == 0){
            built_in_status();
        } else {
            other_commands(curr_command);
        }  
        free_command(curr_command);
    }
    return EXIT_SUCCESS;
}
