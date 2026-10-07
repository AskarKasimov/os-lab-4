#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static void report(const char *role, clock_t start)
{
    double milliseconds = 1000.0 * (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("%s PID=%ld PPID=%ld CPU_ms=%.6f\n",
           role, (long)getpid(), (long)getppid(), milliseconds);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    pid_t first = fork();
    clock_t start = clock();
    if (first == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }
    if (first == 0) {
        report("child1", start);
        return EXIT_SUCCESS;
    }

    pid_t second = fork();
    clock_t second_start = clock();
    if (second == 0) {
        report("child2", second_start);
        return EXIT_SUCCESS;
    }
    int result = EXIT_SUCCESS;
    if (second == -1) {
        perror("fork");
        result = EXIT_FAILURE;
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
    report("main", start);
    return result;
}
