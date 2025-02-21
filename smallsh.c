#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <sys/stat.h>
#include <fcntl.h>
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
        if (token[0] == "#") break;
        
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
    } else if (curr_command->argc == 1){
        chdir(getenv("HOME"));
    } else {
        printf("build_in_cd failed\n");
        return;
    }
}


void built_in_status(){
    printf("exit value: %d\n", status);
}


void other_commands(struct command_line *curr_command) {
    pid_t p = fork();
    
    if (p < 0) {
        perror("Failed Fork");
        exit(1);
    } else if (p == 0) {                                                   // Child process
        if (curr_command->input_file){
            int input = open(curr_command->input_file, O_RDONLY);
            if (input == -1){
                perror("failed to open input file");
                exit(1);
            }
            dup2(input, STDIN_FILENO);
            close(input);
        }

        if (curr_command->output_file) {
            int output = open(curr_command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (output == -1){
                perror("Failed to open output file");
                exit(1);
            }
            dup2(output, STDOUT_FILENO);
            close(output);
        }
        
        if (execvp(curr_command->argv[0], curr_command->argv) == -1) {  
            perror("child command failed");
            exit(1);                                                     // Exit child on failure
        }
    } 
    else {                                                               // Parent process
        if (curr_command->is_bg) {
            printf("Background process started with PID %d\n", p);
            fflush(stdout);
        } 
        else {                                                           // Foreground process handling
            int child_status;
            pid_t w = waitpid(p, &child_status, 0);
            
            if (w == -1) {
                perror("waitpid");
                status = 1;
                exit(1);
            }

            if (WIFEXITED(child_status)) {
                status = WEXITSTATUS(child_status);                     // Store exit status
            } else if (WIFSIGNALED(child_status)) {
                status = WTERMSIG(child_status);                        // Store termination signal
            }
        }
        // Reap background processes
        int wstatus;
        while ((p = waitpid(-1, &wstatus, WNOHANG)) > 0);
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
