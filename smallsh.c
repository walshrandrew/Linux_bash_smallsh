#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> 
#include <signal.h>


#define INPUT_LENGTH 2048
#define MAX_ARGS 512


int status = 0;            // Exit status of last foreground command
bool foreground = false;   // Flag status for foreground-only mode; While True, Background processes disabled


struct command_line
{
    char *argv[MAX_ARGS + 1];
    int argc;
    char *input_file;
    char *output_file;
    bool is_bg;        // Flag status for background processes (&)
};


//FUNCTION: Toggles foreground-only mode if received SIGTSTP
void handle_SIGTSTP(int signo){
    if (!foreground){
        char *message = "Entering foreground-only mode (& is now ignored)";
        write(STDOUT_FILENO, message, 49);
        foreground = 1;
    } else {
        char *message = "\nExiting foreground-only mode\n";
        write(STDOUT_FILENO, message, 29);
        foreground = 0;
    }
}


//FUNCTION: Configures signal handling for SIGTSTP
void signal_handler(){
    struct sigaction SIGTSTP_action = {0};

    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = 0;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
}


//FUNCTION: Frees allocated memory for a command structure
void free_command(struct command_line *curr_command){
    if (curr_command){
        if (curr_command->input_file) free(curr_command->input_file);
        if (curr_command->output_file) free(curr_command->output_file);
        for (int i = 0; i < curr_command->argc; i++) free(curr_command->argv[i]);
        free(curr_command);
    }
}


//Parse user input and store commands
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
        if (token[0] == '#') break;    //ignores comments (#)
        
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
    exit(0);
}


//FUNCTION: Allows user to use "cd" commands
void built_in_cd(struct command_line *curr_command){
    if(curr_command->argc > 1){
        chdir(curr_command->argv[1]);
    } else if (curr_command->argc == 1){
        chdir(getenv("HOME"));
    } else {
        printf("build_in_cd failed\n");
        return;
    }
}


//FUNCTION: Shows last exit status
void built_in_status(){
    printf("exit value: %d\n", status);
}

/*FUNCTION: Supports Input/Output redirection and any non built-in commands
            Supports running commands in foreground and background processes 
*/
void other_commands(struct command_line *curr_command) {
    pid_t p = fork();
    
    if (p < 0) {
        perror("Failed Fork");
        exit(1);
    } else if (p == 0) {                                                   // Child process  
        signal(SIGTSTP, SIG_IGN);
        if (!curr_command->is_bg){
            signal(SIGINT, SIG_DFL);
        }
         
        if (curr_command->input_file){
            int input = open(curr_command->input_file, O_RDONLY);
            if (input == -1){
                fprintf(stderr, "cannot open %s for input\n", curr_command->input_file);
                exit(1);
            }
            dup2(input, STDIN_FILENO);
            close(input);
        }

        if (curr_command->output_file) {
            int output = open(curr_command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (output == -1){
                fprintf(stderr, "cannot open %s for output", curr_command->output_file);
                exit(1);
            }
            dup2(output, STDOUT_FILENO);
            close(output);
        }
        
        if (execvp(curr_command->argv[0], curr_command->argv) == -1) {  
            fprintf(stderr, "%s: no such file or directory\n", curr_command->argv[0]);
            exit(1);                                                     // Exit child on failure
        }
    } 
    else {                                                               // Parent process
        if (curr_command->is_bg && !foreground) {
            printf("background pid is %d\n", p);
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
                printf("terminated by signal %d\n", status);
                fflush(stdout);
            }
        }
    }
}


int main()
{
    signal(SIGINT, SIG_IGN);
    signal_handler();

    struct command_line *curr_command;

    while(true)
    {
        // Reap background processes
        int bgStatus;
        pid_t bgPID;

        while ((bgPID = waitpid(-1, &bgStatus, WNOHANG)) > 0){
            if (WIFEXITED(bgStatus)){
                printf("background pid %d is done: exit value %d\n", bgPID, WEXITSTATUS(bgStatus));
            } else if (WIFSIGNALED(bgStatus)){
                printf("background pid %d is done: terminated by signal %d\n", bgPID, WTERMSIG(bgStatus));
            }
            fflush(stdout);
        }

        curr_command = parse_input();

        if (foreground)
            curr_command->is_bg = false;

        if (curr_command->argc == 0) {
            free_command(curr_command);
            continue;
        }
        // Check for "Built-in" commands
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
