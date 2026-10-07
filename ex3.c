#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s n (0..5)\n", argv[0]);
        return EXIT_FAILURE;
    }
    char *end;
    errno = 0;
    long n = strtol(argv[1], &end, 10);

    if (errno != 0 || end == argv[1] || *end != '\0' || n < 0 || n > 5) {
        fprintf(stderr, "n must be an integer from 0 to 5\n");
        return EXIT_FAILURE;
    }
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("START PID=%ld PPID=%ld n=%ld\n", (long)getpid(), (long)getppid(), n);
    int result = EXIT_SUCCESS;
    for (long i = 0; i < n; ++i) {
        pid_t child = fork();
        if (child == -1) {
            perror("fork");
            result = EXIT_FAILURE;
            break;
        }
        if (child == 0)
            printf("BORN iteration=%ld PID=%ld PPID=%ld\n",
                   i + 1, (long)getpid(), (long)getppid());

        unsigned remaining = 5;
        while ((remaining = sleep(remaining)) != 0) {}
    }
    int status;
    for (;;) {
        pid_t child = wait(&status);
        if (child > 0) {
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
                result = EXIT_FAILURE;
        } else if (errno != EINTR) {
            if (errno != ECHILD) {
                perror("wait");
                result = EXIT_FAILURE;
            }
            break;
        }
    }
    printf("DONE PID=%ld PPID=%ld\n", (long)getpid(), (long)getppid());
    return result;
}
