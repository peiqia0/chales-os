#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/wait.h>

static int failures = 0;

#define CHECK(cond) do { \
    if (cond) { printf("  ok   %s\n", #cond); } \
    else { printf("  FAIL %s (errno=%d)\n", #cond, errno); failures++; } \
} while (0)

static void step(const char *name)
{
    printf("\n[%s] Press Enter to run this test...\n", name);
    fflush(stdout);

    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        /* Wait for the current line to end. */
    }
}

int main(int argc, char **argv)
{
    printf("newlib on chales-os: argc=%d\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("  argv[%d]=%s\n", i, argv[i]);
    }

    step("console");
    CHECK(isatty(1));
    CHECK(getpid() >= 1);

    step("heap");
    char *p = malloc(10000);
    CHECK(p != NULL);
    if (p) {
        memset(p, 'x', 10000);
        free(p);
    }

    char *q = malloc(100);
    CHECK(q != NULL);
    free(q);

    step("files");
    FILE *f = fopen("/nltest.txt", "w");
    CHECK(f != NULL);
    if (f) {
        fprintf(f, "hello %d\n", 42);
        fputs("second line\n", f);
        CHECK(fclose(f) == 0);
    }

    f = fopen("/nltest.txt", "r");
    CHECK(f != NULL);
    if (f) {
        char line[64];
        CHECK(fgets(line, sizeof line, f) &&
              strcmp(line, "hello 42\n") == 0);
        CHECK(fgets(line, sizeof line, f) &&
              strcmp(line, "second line\n") == 0);
        CHECK(fgets(line, sizeof line, f) == NULL);
        CHECK(feof(f));
        CHECK(fseek(f, 6, SEEK_SET) == 0);
        CHECK(fgetc(f) == '4');
        CHECK(ftell(f) == 7);
        fclose(f);
    }

    f = fopen("/nltest.txt", "a");
    if (f) {
        fputs("appended\n", f);
        fclose(f);
    }

    struct stat st;
    CHECK(stat("/nltest.txt", &st) == 0);
    CHECK(S_ISREG(st.st_mode));
    CHECK(st.st_size == 9 + 12 + 9);

    CHECK(fopen("/does-not-exist", "r") == NULL);
    CHECK(errno == ENOENT);
    CHECK(unlink("/nltest.txt") == 0);
    CHECK(stat("/nltest.txt", &st) == -1);

    step("time");
    struct tms t;
    clock_t c = times(&t);
    CHECK(c != (clock_t)-1);

    struct timeval tv;
    CHECK(gettimeofday(&tv, NULL) == 0);

    step("exec and wait");
    int local = 1234;
    char *kept = malloc(32);
    strcpy(kept, "still here");

    char *child_argv[] = { "hello", "from nltest", NULL };
    char *child_envp[] = { NULL };

    CHECK(execve("/bin/hello", child_argv, child_envp) == 0);

    int status = -1;
    pid_t w = wait(&status);
    CHECK(w > 0);
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    CHECK(local == 1234 && strcmp(kept, "still here") == 0);

    errno = 0;
    pid_t w2 = wait(&status);
    printf("  (second wait: ret=%d errno=%d)\n", (int)w2, errno);
    CHECK(w2 == -1 && errno == ECHILD);

    CHECK(execve("/bin/does-not-exist", child_argv, child_envp) == -1);
    free(kept);

    step("stdin");
    printf("type a line: ");
    fflush(stdout);

    char buf[80];
    if (fgets(buf, sizeof buf, stdin)) {
        printf("you typed: %s", buf);
    }

    printf("%s\n", failures ? "SOME TESTS FAILED" : "ALL TESTS PASSED");
    return failures ? 1 : 0;
}
