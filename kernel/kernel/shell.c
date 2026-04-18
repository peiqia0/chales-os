#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <kernel/keyboard.h>
#include <kernel/tty.h>
#include <kernel/ramfs.h>

#define SHELL_BUFFER_SIZE 256
#define SHELL_MAX_ARGS 16

/* Shell input buffer */
static char shell_input_buffer[SHELL_BUFFER_SIZE];

/* Command structure */
typedef struct {
    const char *name;
    const char *description;
} shell_command_t;


/* Forward declarations */
static int cmd_help(int argc, char *argv[]);
static int cmd_clear(int argc, char *argv[]);
static int cmd_echo(int argc, char *argv[]);
static int cmd_ls(int argc, char *argv[]);
static int cmd_cat(int argc, char *argv[]);
static int cmd_touch(int argc, char *argv[]);
static int cmd_reboot(int argc, char *argv[]);
static int cmd_uname(int argc, char *argv[]);
static int cmd_mkdir(int argc, char *argv[]);
static int cmd_rmdir(int argc, char *argv[]);
static int cmd_rm(int argc, char *argv[]);
static int cmd_write(int argc, char *argv[]);

/* Command handlers */
typedef int (*cmd_handler_t)(int argc, char *argv[]);

typedef struct {
    const char *name;
    cmd_handler_t handler;
    const char *description;
} shell_cmd_entry_t;

static shell_cmd_entry_t commands[] = {
    {"help", cmd_help, "Show available commands"},
    {"clear", cmd_clear, "Clear the screen"},
    {"echo", cmd_echo, "Print text"},
    {"ls", cmd_ls, "List files in directory"},
    {"cat", cmd_cat, "Display file contents"},
    {"touch", cmd_touch, "Create an empty file"},
    {"reboot", cmd_reboot, "Reboot the system"},
    {"uname", cmd_uname, "Display system information"},
    {"mkdir", cmd_mkdir, "Create a directory"},
    {"rmdir", cmd_rmdir, "Remove a directory"},
    {"rm", cmd_rm, "Delete a file"},
    {"write", cmd_write, "Write text to a file"},
    {NULL, NULL, NULL}
};

/* ========== Command implementations ========== */

static int cmd_help(int _argc, char *_argv[])
{
    (void)_argc; (void)_argv;
    printf("Available commands:\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        printf("  %s - %s\n", commands[i].name, commands[i].description);
    }
    return 0;
}
static int cmd_write(int argc, char *argv[])
{
    if (argc < 3) {
        printf("Usage: write <file> <text>\n");
        return 1;
    }

    int fd = ramfs_open(argv[1], 1);
    if (fd < 0) {
        printf("Error: cannot open '%s'\n", argv[1]);
        return 1;
    }

    for (int i = 2; i < argc; i++) {
        ramfs_write(fd, argv[i], strlen(argv[i]));
        if (i < argc - 1)
            ramfs_write(fd, " ", 1);
    }

    ramfs_close(fd);
    return 0;
}

static int cmd_rm(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: rm <filename>\n");
        return 1;
    }

    if (ramfs_unlink(argv[1]) < 0) {
        printf("Error: cannot delete '%s'\n", argv[1]);
        return 1;
    }

    return 0;
}

static int cmd_clear(int _argc, char *_argv[])
{
    (void)_argc; (void)_argv;
    terminal_initialize();
    return 0;
}
static int cmd_mkdir(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: mkdir <dirname>\n");
        return 1;
    }

    if (ramfs_mkdir(argv[1]) < 0) {
        printf("Error: cannot create directory '%s'\n", argv[1]);
        return 1;
    }

    return 0;
}
static int cmd_rmdir(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: rmdir <dirname>\n");
        return 1;
    }

    if (ramfs_rmdir(argv[1]) < 0) {
        printf("Error: cannot remove directory '%s'\n", argv[1]);
        return 1;
    }

    return 0;
}

static int cmd_echo(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
        if (i < argc - 1) printf(" ");
    }
    printf("\n");
    return 0;
}
static int cmd_ls(int argc, char *argv[])
{
    const char *path = "/";

    if (argc >= 2) {
        path = argv[1];
    }

    ramfs_ls(path);
    return 0;
}


static int cmd_cat(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: cat <filename>\n");
        return 1;
    }

    int fd = ramfs_open(argv[1], 0);
    if (fd < 0) {
        printf("Error: cannot open file '%s'\n", argv[1]);
        return 1;
    }

    char buffer[256];
    int bytes;

    while ((bytes = ramfs_read(fd, buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < bytes; i++) {
            putchar(buffer[i]);
        }
    }

    printf("\n");

    ramfs_close(fd);
    return 0;
}


static int cmd_touch(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: touch <filename>\n");
        return 1;
    }

    int fd = ramfs_open(argv[1], 1);
    if (fd < 0) {
        printf("Error: cannot create file '%s'\n", argv[1]);
        return 1;
    }

    ramfs_close(fd);
    return 0;
}


static int cmd_reboot(int _argc, char *_argv[])
{
    (void)_argc; (void)_argv;
    printf("Rebooting...\n");
    /* Triple fault CPU reset */
    asm volatile("lidt (%0)" : : "r"((void *)0));
    asm volatile("int $3");
    return 0;
}

static int cmd_uname(int _argc, char *_argv[])
{
    (void)_argc; (void)_argv;
    printf("System Information:\n");
    printf("  Kernel: Charles OS 0.1.5\n");
    printf("  Architecture: i386\n");
    printf("  Compiler: GCC (cross-compiler)\n");
    return 0;
}


// Read a line of input from the keyboard buffer
static int shell_read_line(char *buffer, size_t max_size)
{
    int index = 0;
    
    while (index < (int)max_size - 1) {
        if (keyboard_buffer_has_data()) {
            char c = keyboard_buffer_pop();
            
            if (c == '\n') {
                buffer[index] = '\0';
                printf("\n");
                return index;
            } else if (c == '\b') {
                if (index > 0) {
                    index--;
                    printf("\b \b");
                }
            } else if (c >= 32 && c < 127) {
                buffer[index++] = c;
                putchar(c);
            }
        }
    }
    
    buffer[index] = '\0';
    return index;
}

// Parse a command line into arguments

static int shell_parse_command(char *line, char *argv[], int max_args)
{
    int argc = 0;
    char *p = line;
    
    while (*p && argc < max_args) {
        /* Skip whitespace */
        while (*p && (*p == ' ' || *p == '\t')) {
            p++;
        }
        
        if (*p == '\0') break;
        
        /* Argument starts here */
        argv[argc++] = p;
        
        /* Find end of argument */
        while (*p && *p != ' ' && *p != '\t') {
            p++;
        }
        
        /* Null-terminate argument */
        if (*p) {
            *p++ = '\0';
        }
    }
    
    return argc;
}

// Execute a shell command

static int shell_execute_command(int argc, char *argv[])
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

// Shell prompt and main loop

void shell_run(void)
{
    printf("\n=== Charles OS Shell ===\n");
    printf("Type 'help' for available commands\n\n");
    
    while (1) {
        printf("> ");
        
        shell_read_line(shell_input_buffer, SHELL_BUFFER_SIZE);
        
        char *argv[SHELL_MAX_ARGS];
        int argc = shell_parse_command(shell_input_buffer, argv, SHELL_MAX_ARGS);
        
        if (argc > 0) {
            shell_execute_command(argc, argv);
        }
    }
}
