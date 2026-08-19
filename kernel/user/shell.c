#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscalls.h>

#define SHELL_BUFFER_SIZE 256
#define SHELL_MAX_ARGS 16
#define CAT_BUF_SIZE 256

static char shell_input_buffer[SHELL_BUFFER_SIZE];

typedef int (*cmd_handler_t)(int argc, char* argv[]);

typedef struct {
    const char* name;
    cmd_handler_t handler;
    const char* description;
} shell_cmd_entry_t;

static int cmd_help(int argc, char* argv[]);
static int cmd_clear(int argc, char* argv[]);
static int cmd_echo(int argc, char* argv[]);
static int cmd_ls(int argc, char* argv[]);
static int cmd_cat(int argc, char* argv[]);
static int cmd_touch(int argc, char* argv[]);
static int cmd_write(int argc, char* argv[]);
static int cmd_mkdir(int argc, char* argv[]);
static int cmd_rmdir(int argc, char* argv[]);
static int cmd_rm(int argc, char* argv[]);
static int cmd_uname(int argc, char* argv[]);
static int cmd_reboot(int argc, char* argv[]);
static int cmd_exec(int argc, char* argv[]);

static shell_cmd_entry_t commands[] = {
    {"help",   cmd_help,   "Show available commands"},
    {"clear",  cmd_clear,  "Clear the screen"},
    {"echo",   cmd_echo,   "Print text"},
    {"ls",     cmd_ls,     "List files in directory"},
    {"cat",    cmd_cat,    "Display file contents"},
    {"touch",  cmd_touch,  "Create an empty file"},
    {"write",  cmd_write,  "Write text to a file"},
    {"mkdir",  cmd_mkdir,  "Create a directory"},
    {"rmdir",  cmd_rmdir,  "Remove a directory"},
    {"rm",     cmd_rm,     "Delete a file"},
    {"uname",  cmd_uname,  "Display system information"},
    {"reboot", cmd_reboot, "Reboot the system"},
    {"exec",   cmd_exec,   "Load and run an ELF binary"},
    {NULL, NULL, NULL}
};


static int cmd_help(int argc, char* argv[])
{
    (void)argc; (void)argv;
    printf("Available commands:\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        printf("  %s - %s\n", commands[i].name, commands[i].description);
    }
    return 0;
}

static int cmd_clear(int argc, char* argv[])
{
    (void)argc; (void)argv;
    _syscall_clear();
    return 0;
}

static int cmd_echo(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
        if (i < argc - 1) printf(" ");
    }
    printf("\n");
    return 0;
}

static int cmd_ls(int argc, char* argv[])
{
    const char* path = "";
    if (argc >= 2) {
        path = argv[1];
    }
    _syscall_ls(path);
    return 0;
}

static int cmd_cat(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: cat <filename>\n");
        return 1;
    }

    int fd = _syscall_open(argv[1], 0);
    if (fd < 0) {
        printf("Error: cannot open file '%s'\n", argv[1]);
        return 1;
    }

    char* buffer = malloc(CAT_BUF_SIZE);
    if (!buffer) {
        printf("Error: out of memory\n");
        _syscall_close(fd);
        return 1;
    }

    int bytes;
    while ((bytes = _syscall_read(fd, buffer, CAT_BUF_SIZE)) > 0) {
        for (int i = 0; i < bytes; i++) {
            printf("%c", buffer[i]);
        }
    }
    printf("\n");

    free(buffer);
    _syscall_close(fd);
    return 0;
}

static int cmd_touch(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: touch <filename>\n");
        return 1;
    }

    int fd = _syscall_open(argv[1], 1 /* O_CREAT */);
    if (fd < 0) {
        printf("Error: cannot create file '%s'\n", argv[1]);
        return 1;
    }

    _syscall_close(fd);
    return 0;
}

static int cmd_write(int argc, char* argv[])
{
    if (argc < 3) {
        printf("Usage: write <file> <text>\n");
        return 1;
    }

    int fd = _syscall_open(argv[1], 1 /* O_CREAT */);
    if (fd < 0) {
        printf("Error: cannot open '%s'\n", argv[1]);
        return 1;
    }

    for (int i = 2; i < argc; i++) {
        _syscall_write(fd, argv[i], (unsigned int)strlen(argv[i]));
        if (i < argc - 1) {
            _syscall_write(fd, " ", 1);
        }
    }

    _syscall_close(fd);
    return 0;
}

static int cmd_mkdir(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: mkdir <dirname>\n");
        return 1;
    }
    if (_syscall_mkdir(argv[1]) < 0) {
        printf("Error: cannot create directory '%s'\n", argv[1]);
        return 1;
    }
    return 0;
}

static int cmd_rmdir(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: rmdir <dirname>\n");
        return 1;
    }
    if (_syscall_rmdir(argv[1]) < 0) {
        printf("Error: cannot remove directory '%s'\n", argv[1]);
        return 1;
    }
    return 0;
}

static int cmd_rm(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: rm <filename>\n");
        return 1;
    }
    if (_syscall_unlink(argv[1]) < 0) {
        printf("Error: cannot delete '%s'\n", argv[1]);
        return 1;
    }
    return 0;
}

static int cmd_uname(int argc, char* argv[])
{
    (void)argc; (void)argv;
    printf("System Information:\n");
    printf("  Kernel: chales-os\n");
    printf("  Architecture: i386\n");
    printf("  Compiler: GCC (cross-compiler)\n");
    return 0;
}

static int cmd_reboot(int argc, char* argv[])
{
    (void)argc; (void)argv;
    printf("Rebooting...\n");
    _syscall_reboot(); // does not return
    return 0;
}

static int cmd_exec(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: exec <path>\n");
        return 1;
    }

    /* On success this never returns to us — control passes straight to
     * the loaded program in ring 3. We only get here on failure. */
    _syscall_exec(argv[1]);
    printf("exec: failed to run '%s'\n", argv[1]);
    return 1;
}


/* Blocks (busy-polls _syscall_getchar) until a full line is typed. */
static int shell_read_line(char* buffer, size_t max_size)
{
    int index = 0;

    while (index < (int)max_size - 1) {
        char c = (char)_syscall_getchar();
        if (c == '\0') {
            continue; // no key waiting yet
        }

        if (c == '\n') {
            buffer[index] = '\0';
            printf("\n");
            return index;
        } else if (c == '\b') {
            if (index > 0) {
                index--;
                printf("\b"); // tty.c both moves the cursor back and blanks it
            }
        } else if (c >= 32 && c < 127) {
            buffer[index++] = c;
            printf("%c", c);
        }
    }

    buffer[index] = '\0';
    return index;
}

static int shell_parse_command(char* line, char* argv[], int max_args)
{
    int argc = 0;
    char* p = line;

    while (*p && argc < max_args) {
        while (*p && (*p == ' ' || *p == '\t')) {
            p++;
        }
        if (*p == '\0') break;

        argv[argc++] = p;

        while (*p && *p != ' ' && *p != '\t') {
            p++;
        }
        if (*p) {
            *p++ = '\0';
        }
    }

    return argc;
}

static int shell_execute_command(int argc, char* argv[])
{
    if (argc == 0) {
        return 0;
    }

    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            return commands[i].handler(argc, argv);
        }
    }

    printf("Unknown command: %s\n", argv[0]);
    return 127;
}

void shell_run(void)
{
    printf("\nchales-os shell\n");
    printf("Type 'help' for available commands\n\n");

    while (1) {
        printf("> ");

        shell_read_line(shell_input_buffer, SHELL_BUFFER_SIZE);

        char* argv[SHELL_MAX_ARGS];
        int argc = shell_parse_command(shell_input_buffer, argv, SHELL_MAX_ARGS);

        if (argc > 0) {
            shell_execute_command(argc, argv);
        }
    }
}
