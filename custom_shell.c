/*
    Author: Madhavan Vaidya
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <stdio.h>

// Global Variables

#define MAX_PIPES 8
#define MAX_ARGS 5
#define MAX_CMD_LENGTH 560

char commands[8][200];

int cmd_length;
int pipe_communication[2];
int input_cmd_count;

// Parse Commands with ~

int tilde_parse_command(char *modified_cmd, char **cmd_args)
{
    int argc = 0;
    char *tokens = strtok(modified_cmd, " \t\n");

    while (tokens != NULL)
    {

        if (tokens[0] == '~')
        {
            char *expanded_token = malloc(strlen(tokens) + strlen(getenv("HOME")) + 1);
            strcpy(expanded_token, getenv("HOME"));
            strcat(expanded_token, tokens + 1);
            cmd_args[argc] = expanded_token;
        }
        else
        {
            cmd_args[argc] = tokens;
        }

        argc++;

        tokens = strtok(NULL, " \t\n");
    }

    cmd_args[argc] = NULL;

    return argc;
}

// Tokenize the commands based on White Spaces

void split_cmd(char *hold_cmds[50], char *input_str)
{
    int cmd_track = 0;
    char *re_tokens = strtok(input_str, " ");

    while (true)

    {
        if (re_tokens == NULL)

        {
            hold_cmds[cmd_track] = NULL;

            cmd_track++;

            break;
        }

        hold_cmds[cmd_track] = re_tokens;
        cmd_track++;
        re_tokens = strtok(NULL, " ");
    }

    if (cmd_track < 1 || cmd_track > 5)

    {
        puts("The argument count for each comand should be in range of >=1 and <=5");
        exit(10);
    }
}

// Different Methods for Command Parsing

void execute_symbol(char *user_given_console_cmd)
{
    for (int cp = 0; cp < cmd_length; cp++)

    {
        // Check for concatenation operation #
        if (user_given_console_cmd[cp] == '#')
        {
            concatenate_files(user_given_console_cmd);
            exit(1);
        }

        if (user_given_console_cmd[cp] == '|')

        {
            if (user_given_console_cmd[cp + 1] == '|')

            {
                conditional_and_or(user_given_console_cmd);
                exit(1);
            }

            pipe_operation(user_given_console_cmd);
            exit(1);
        }

        if (user_given_console_cmd[cp] == '&')
        {
            if (user_given_console_cmd[cp + 1] == '&')

            {
                conditional_and_or(user_given_console_cmd);
                exit(1);
            }

            background_process(user_given_console_cmd);

            exit(1);
        }

        if (user_given_console_cmd[cp] == '>')

        {
            if (user_given_console_cmd[cp + 1] == '>')

            {
                append_output(user_given_console_cmd);
                exit(1);
            }
            else
            {
                redirect_output(user_given_console_cmd);
            }
            exit(1);
        }
        if (user_given_console_cmd[cp] == '<')

        {
            input_redirect(user_given_console_cmd);
            exit(1);
        }

        if (user_given_console_cmd[cp] == ';')
        {
            sequential_command(user_given_console_cmd);
            exit(1);
        }
    }

    pipe_operation(user_given_console_cmd);
}

// Concatenate files based on '#'
void concatenate_files(char *user_given_console_cmd)
{
    char *file_names[5];
    int file_count = 0;

    char *token = strtok(user_given_console_cmd, "#");
    while (token != NULL && file_count < 5)
    {
        while (*token == ' ')
            token++;
        int len = strlen(token);
        while (len > 0 && token[len - 1] == ' ')
            token[--len] = '\0';

        file_names[file_count++] = token;
        token = strtok(NULL, "#");
    }

    for (int i = 0; i < file_count; ++i)
    {

        int fd = open(file_names[i], O_RDONLY);
        if (fd == -1)
        {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }

        char buffer[1024];
        int bytes_read;
        while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
        {
            write(STDOUT_FILENO, buffer, bytes_read);
        }

        close(fd);
    }
}

// Pipe operation using |

void pipe_operation(char *command)
{
    char *arguments[MAX_ARGS + 2];
    char *pipe_cmds[MAX_PIPES + 1];

    int pipe_count = 0;

    char *tokens = strtok(command, "|");

    while (tokens != NULL)
    {

        while (*tokens && (*tokens == ' ' || *tokens == '\t'))

            ++tokens;

        size_t len = strlen(tokens);

        while (len > 0 && (tokens[len - 1] == ' ' || tokens[len - 1] == '\t'))

            tokens[--len] = '\0';

        pipe_cmds[pipe_count++] = tokens;

        tokens = strtok(NULL, "|");
    }

    pipe_cmds[pipe_count] = NULL;

    if (pipe_count > 0 && pipe_count <= MAX_PIPES)
    {
        int prev_pipe[2];

        for (int i = 0; i < pipe_count; ++i)
        {
            char *current_command = pipe_cmds[i];

            int argc = tilde_parse_command(current_command, arguments);
            if (argc > 0 && argc <= MAX_ARGS)
            {
                int pipe_fd[2];

                if (pipe(pipe_fd) < 0)
                {
                    perror("Pipe Creation Failed!");

                    exit(EXIT_FAILURE);
                }

                pid_t pipe_id = fork();

                if (pipe_id < 0)
                {
                    perror("Fork failed");
                    exit(EXIT_FAILURE);
                }
                else if (pipe_id == 0)
                {
                    if (i > 0)
                    {

                        close(prev_pipe[1]);

                        dup2(prev_pipe[0], STDIN_FILENO);

                        close(prev_pipe[0]);
                    }

                    if (i < pipe_count - 1)
                    {
                        close(pipe_fd[0]);
                        dup2(pipe_fd[1], STDOUT_FILENO);
                        close(pipe_fd[1]);
                    }

                    execvp(arguments[0], arguments);
                    perror("Execution failed");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    if (i > 0)
                    {
                        close(prev_pipe[0]);
                        close(prev_pipe[1]);
                    }

                    prev_pipe[0] = pipe_fd[0];
                    prev_pipe[1] = pipe_fd[1];

                    int child_status;

                    waitpid(pipe_id, &child_status, 0);

                    if (WIFEXITED(child_status))
                    {

                        if (i == pipe_count - 1)
                        {
                        }
                    }
                }
            }
            else
            {
                printf("Invalid number of arguments!\n");
                break;
            }
        }

        if (pipe_count > 1)
        {
            close(prev_pipe[0]);
            close(prev_pipe[1]);
        }
    }
    else
    {
        printf("Invalid number of commands!\n");
    }
}

// Redirection based on >

void redirect_output(char *user_given_console_cmd)
{
    char *command = strtok(user_given_console_cmd, ">");
    char *output_file = strtok(NULL, ">");
    if (!command || !output_file)
    {
        fprintf(stderr, "Invalid syntax for redirection\n");
        exit(EXIT_FAILURE);
    }

    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1)
    {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    }

    int child_pid = fork();
    if (child_pid == 0)
    {
        dup2(fd, STDOUT_FILENO);
        close(fd);
        execute_symbol(command);
        exit(EXIT_SUCCESS);
    }
    else if (child_pid > 0)
    {
        wait(NULL);
    }
    else
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
}

// Append Output Operation using >>
void append_output(char *user_given_console_cmd) {
    char *command = strtok(user_given_console_cmd, ">>");
    char *output_file = strtok(NULL, ">>");
    if (!command || !output_file) {
        fprintf(stderr, "Invalid syntax for append redirection\n");
        exit(EXIT_FAILURE);
    }

    // Open the output file in append mode
    int fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    }

    // Redirect stdout to the output file
    if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("Error redirecting stdout");
        exit(EXIT_FAILURE);
    }

    // Close the file descriptor as it's no longer needed
    close(fd);
    system(command);
    exit(EXIT_SUCCESS);
}

// Input Redirection Operation

void input_redirect(char *user_given_console_cmd)
{
    int size = 0;
    for (int i = 0; i < cmd_length; i++)
    {

        if (user_given_console_cmd[i] == '<')
        {
            commands[input_cmd_count][size] = '\0';

            input_cmd_count++;

            size = 0;
        }
        else
        {

            commands[input_cmd_count][size] = user_given_console_cmd[i];

            size++;
        }
    }

    commands[input_cmd_count][size] = '\0';

    input_cmd_count++;

    if (input_cmd_count != 2)
    {
        fprintf(stderr, "Error: Input redirection requires two commands.\n");

        exit(EXIT_FAILURE);
    }

    int file_desc;

    int pipe_1[2];

    if (pipe(pipe_1) == -1)
    {

        perror("Failing of Pipe Communication Creation");

        exit(EXIT_FAILURE);
    }

    int child_pid = fork();

    if (child_pid == 0)
    {

        char *cmd_arg1[50];

        char *cmd_arg2[50];

        split_cmd(cmd_arg1, commands[0]);

        split_cmd(cmd_arg2, commands[1]);

        file_desc = open(cmd_arg2[0], O_RDONLY);

        if (file_desc == -1)
        {

            perror("Input file opening failed");

            exit(EXIT_FAILURE);
        }

        dup2(file_desc, 0);

        dup2(pipe_1[1], 1);
        close(pipe_1[0]);
        close(pipe_1[1]);
        close(file_desc);

        execvp(cmd_arg1[0], cmd_arg1);

        fprintf(stderr, "command couldnt be executed\n");

        exit(EXIT_FAILURE);
    }
    else if (child_pid > 0)
    {
        close(pipe_1[1]);

        wait(NULL);

        char buffer[1024];

        int bytes_read;

        while ((bytes_read = read(pipe_1[0], buffer, sizeof(buffer))) > 0)
        {
            write(STDOUT_FILENO, buffer, bytes_read);
        }

        close(pipe_1[0]);
    }
    else
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
}

int cmd_count_for_process;

char *cmds[256];

// Conditional Execution based on && or ||

void conditional_and_or(char *user_given_console_cmd)
{
    char *tokens;

    char *split_tokens;

    if (strstr(user_given_console_cmd, "&&") != NULL)
    {
        tokens = strtok(user_given_console_cmd, "&&");
        split_tokens = "&&";
    }

    else if (strstr(user_given_console_cmd, "||") != NULL)
    {
        tokens = strtok(user_given_console_cmd, "||");
        split_tokens = "||";
    }
    else
    {

        printf("Invalid input! Operator '&&' or '||' not found");

        exit(1);
    }

    while (tokens != NULL)
    {
        cmds[cmd_count_for_process++] = tokens;
        tokens = strtok(NULL, split_tokens);
    }

    if (cmd_count_for_process < 1 || cmd_count_for_process > 6)
    {
        printf("Invalid number of commands given");
        exit(1);
    }

    int child_exit_status;

    for (int i = 0; i < cmd_count_for_process; i++)

    {
        int run_logical_operation = fork();

        if (run_logical_operation == 0)

        {
            execlp("sh", "sh", "-c", cmds[i], NULL);

            exit(1);
        }

        else if (run_logical_operation > 0)

        {
            wait(&child_exit_status);

            if (strcmp(split_tokens, "&&") == 0)
            {
                if (WIFEXITED(child_exit_status) && WEXITSTATUS(child_exit_status) != 0) // command failed

                {
                    break;
                }
            }

            else if (strcmp(split_tokens, "||") == 0)

            {
                if (WIFEXITED(child_exit_status) && WEXITSTATUS(child_exit_status) == 0)
                {
                    break;
                }
            }
        }
    }
}

// Background Process Execution using 'bg'

void background_process(char *user_given_console_cmd)
{
    int individual_command_count = 0;

    for (int bp = 0; bp < cmd_length; bp++)

    {
        if (user_given_console_cmd[bp] == '&')
        {
            if ((bp + 1) != cmd_length)

            {

                printf("Error in given input command. Please dont put anything after &, not even space");

                exit(1);
            }

            commands[input_cmd_count][individual_command_count] = ' ';
            commands[input_cmd_count][individual_command_count + 1] = '&';
            commands[input_cmd_count][individual_command_count + 2] = '\0';
            input_cmd_count++;
            individual_command_count = 0;
        }

        else

        {

            commands[input_cmd_count][individual_command_count] = user_given_console_cmd[bp];

            individual_command_count++;
        }
    }

    int bg_process_id = fork();

    if (bg_process_id == 0)
    {

        char *command_args[50];
        split_cmd(command_args, commands[0]);

        printf("\n");

        close(0);
        close(1);
        close(pipe_communication[0]);
        close(pipe_communication[1]);
        execvp(command_args[0], command_args);

        exit(EXIT_FAILURE);
    }

    else if (bg_process_id > 0)

    {
        close(pipe_communication[0]);
        close(pipe_communication[1]);
        wait(NULL);

        exit(1);
    }

    exit(EXIT_FAILURE);
}

// Sequential Execution

void sequential_command(char *user_given_console_cmd)
{
    int command_length = 0;
    for (int i = 0; i < cmd_length; i++)

    {
        if (user_given_console_cmd[i] == ';')

        {
            commands[input_cmd_count][command_length] = '\0';
            input_cmd_count++;
            command_length = 0;
        }

        else

        {
            commands[input_cmd_count][command_length] = user_given_console_cmd[i];
            command_length++;
        }
    }

    commands[input_cmd_count][command_length] = '\0';

    input_cmd_count++;

    if (input_cmd_count < 1 || input_cmd_count > 7)

    {
        printf("You can only pass upto 7 commands");

        exit(1);
    }

    int child_exit_status;

    for (int cmnds = 0; cmnds < input_cmd_count; cmnds++)

    {
        int child_pid = fork();

        if (child_pid == 0)

        {
            char *seq_command[50];
            split_cmd(seq_command, commands[cmnds]);
            execvp(seq_command[0], seq_command);

            exit(1);
        }

        else if (child_pid > 0)

        {
            waitpid(child_pid, &child_exit_status, 0);

            if (WIFEXITED(child_exit_status) && WEXITSTATUS(child_exit_status) != 0)

            {
                close(pipe_communication[0]);
                close(pipe_communication[1]);

                exit(1);
            }
        }
    }

    close(pipe_communication[0]);
    close(pipe_communication[1]);

    exit(1);
}

// Main Function

int main()
{
    int execute_current_function;

    char console_user_input[MAX_CMD_LENGTH];

    while (true)

    {
        printf("shell24$ ");

        fgets(console_user_input, MAX_CMD_LENGTH, stdin);
        console_user_input[strcspn(console_user_input, "\n")] = '\0';

        if (strcmp(console_user_input, "newt") == 0)
        {
            system("mate-terminal -e ./shell24 &");
            printf("New shell24 created.\n");
        }

        else
        {
            cmd_length = strlen(console_user_input);
            execute_current_function = fork();

            if (execute_current_function == 0)
            {
                execute_symbol(console_user_input);
                exit(1);
            }
            else if (execute_current_function >= 1)
            {
                char temp_string_cmd[MAX_CMD_LENGTH];
                int ppc = read(pipe_communication[0], temp_string_cmd, MAX_CMD_LENGTH);

                temp_string_cmd[ppc] = '\0';
            }
            else
            {
                exit(1);
            }
        }
    }
    return 0;
}
