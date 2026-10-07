#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t waiting_for_input = 0;

static void reap_children(int signal_number)
{
    (void)signal_number;
    int saved_errno = errno;
    int finished = 0;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        finished = 1;
    if (finished && waiting_for_input)
        write(STDOUT_FILENO, "\nlab-shell> ", sizeof("\nlab-shell> ") - 1);
    errno = saved_errno;
}

static void wait_for_children(void)
{
    while (waitpid(-1, NULL, 0) != -1 || errno == EINTR) {}
}

int main(void)
{
    struct sigaction action = {0};
    action.sa_handler = reap_children;
    action.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGCHLD, &action, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }
    setvbuf(stdout, NULL, _IONBF, 0);
    char *line = NULL;
    size_t capacity = 0;
    int interactive = isatty(STDIN_FILENO);
    sigset_t blocked, previous;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGCHLD);
    for (;;) {
        sigprocmask(SIG_BLOCK, &blocked, &previous);
        waiting_for_input = interactive;
        if (interactive)
            printf("lab-shell> ");
        sigprocmask(SIG_SETMASK, &previous, NULL);
        ssize_t length = getline(&line, &capacity, stdin);
        waiting_for_input = 0;
        if (length == -1)
            break;

        char *args[128];
        size_t count = 0;
        char *save;
        char *token = strtok_r(line, " \t\r\n", &save);
        while (token != NULL && count < 127) {
            args[count++] = token;
            token = strtok_r(NULL, " \t\r\n", &save);
        }
        if (token != NULL) {
            fprintf(stderr, "Too many arguments (maximum 127 tokens)\n");
            continue;
        }
        args[count] = NULL;
        if (count == 0)
            continue;
        if (strcmp(args[0], "exit") == 0)
            break;
        if (strcmp(args[0], "wait") == 0) {
            wait_for_children();
            continue;
        }
        pid_t child = fork();
        if (child == -1) {
            perror("fork");
            continue;
        }
        if (child == 0) {
            int null_input = open("/dev/null", O_RDONLY);
            if (null_input == -1 || dup2(null_input, STDIN_FILENO) == -1) {
                perror("background stdin");
                _exit(126);
            }
            if (null_input != STDIN_FILENO)
                close(null_input);
            execvp(args[0], args);
            perror(args[0]);
            _exit(127);
        }
        printf("started PID=%ld command=%s\n", (long)child, args[0]);
    }
    int result = ferror(stdin) ? EXIT_FAILURE : EXIT_SUCCESS;
    if (result != EXIT_SUCCESS)
        perror("getline");
    free(line);
    wait_for_children();
    return result;
}
