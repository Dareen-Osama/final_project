#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#define SHELL_INPUT_MAX 1024
#define MAX_ARGS 100

extern char **environ;

void show_intro() {
    printf("----------------OUR SHELL--------------------\n");
    printf("----------- DAREEN, LEENA , NOURA --------------\n");
    printf("....Type 'help' for entire user manual\n");
    printf("to show the information about the command you want....\n\n");
}

char *get_input(FILE *source) {
    char *input = malloc(SHELL_INPUT_MAX);
    if (fgets(input, SHELL_INPUT_MAX, source) == NULL) {
        free(input);
        return NULL;
    }
    input[strcspn(input, "\n")] = '\0';
    return input;
}

void parse_redirect(char **args, char **input_file, char **output_file, int *append) {
    for (int i = 0; args[i]; i++) {
        if (strcmp(args[i], "<") == 0) {
            *input_file = args[i + 1];
            args[i] = NULL;
        } else if (strcmp(args[i], ">") == 0) {
            *output_file = args[i + 1];
            *append = 0;
            args[i] = NULL;
        } else if (strcmp(args[i], ">>") == 0) {
            *output_file = args[i + 1];
            *append = 1;
            args[i] = NULL;
        }
    }
}

int handle_internal_commands(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1]) chdir(args[1]);
        else {
            char cwd[1024];
            getcwd(cwd, sizeof(cwd));
            printf("%s\n", cwd);
        }
        return 1;
    } else if (strcmp(args[0], "clr") == 0) {
        printf("\033[H\033[J");
        return 1;
    } else if (strcmp(args[0], "dir") == 0) {
        char *path = args[1] ? args[1] : ".";
        DIR *d = opendir(path);
        if (!d) { perror("dir"); return 1; }
        struct dirent *entry;
        while ((entry = readdir(d)) != NULL) {
            printf("%s\n", entry->d_name);
        }
        closedir(d);
        return 1;
    } else if (strcmp(args[0], "environ") == 0) {
        for (char **env = environ; *env != NULL; env++) {
            printf("%s\n", *env);
        }
        return 1;
    } else if (strcmp(args[0], "echo") == 0) {
        for (int i = 1; args[i] != NULL; i++) {
            printf("%s ", args[i]);
        }
        printf("\n");
        return 1;
    } else if (strcmp(args[0], "help") == 0) {
        FILE *fp = fopen("readme", "r");
        if (!fp) { perror("help"); return 1; }
        char c;
        while ((c = fgetc(fp)) != EOF) putchar(c);
        fclose(fp);
        return 1;
    } else if (strcmp(args[0], "pause") == 0) {
        printf("Shell paused. Press Enter to continue...");
        while (getchar() != '\n');
        return 1;
    } else if (strcmp(args[0], "quit") == 0) {
        exit(0);
    }
    return 0;
}

void redirect_io(char **args, int *in_fd, int *out_fd) {
    for (int i = 0; args[i]; i++) {
        if (strcmp(args[i], "<") == 0) {
            *in_fd = open(args[i + 1], O_RDONLY);
            args[i] = NULL;
        } else if (strcmp(args[i], ">") == 0) {
            *out_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            args[i] = NULL;
        } else if (strcmp(args[i], ">>") == 0) {
            *out_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            args[i] = NULL;
        }
    }
}

void execute_command(char *input) {
    char *args[MAX_ARGS];
    char *token = strtok(input, " \t");
    int arg_index = 0;
    while (token) {
        args[arg_index++] = token;
        token = strtok(NULL, " \t");
    }
    args[arg_index] = NULL;

    if (args[0] == NULL) return;

    if (handle_internal_commands(args)) return;

    pid_t pid = fork();
    if (pid == 0) {
        setenv("parent", getenv("shell"), 1);
        int in_fd = STDIN_FILENO, out_fd = STDOUT_FILENO;
        redirect_io(args, &in_fd, &out_fd);
        dup2(in_fd, STDIN_FILENO);
        dup2(out_fd, STDOUT_FILENO);
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        wait(NULL);
    } else {
        perror("fork");
    }
}

void parse_and_execute(char *input) {
    if (strlen(input) == 0) return;
    if (strchr(input, '&')) {
        pid_t pid = fork();
        if (pid == 0) {
            execute_command(strtok(input, "&"));
            exit(0);
        }
    } else {
        execute_command(input);
    }
}

int main(int argc, char *argv[]) {
    FILE *input_source = stdin;
    char *input;

    // Set shell environment variable
    char shell_path[1024];
    getcwd(shell_path, sizeof(shell_path));
    strcat(shell_path, "/myshell");
    setenv("shell", shell_path, 1);

    if (argc == 2) {
        input_source = fopen(argv[1], "r");
        if (!input_source) {
            perror("Batch file error");
            exit(EXIT_FAILURE);
        }
    } else {
        show_intro();
    }

    while (1) {
        if (input_source == stdin) {
            char cwd[1024];
            getcwd(cwd, sizeof(cwd));
            printf("%s> ", cwd);
        }
        input = get_input(input_source);
        if (!input) break;
        parse_and_execute(input);
        free(input);
    }

    if (input_source != stdin) fclose(input_source);
    return 0;
}

