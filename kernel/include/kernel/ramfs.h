#ifndef _KERNEL_RAMFS_H
#define _KERNEL_RAMFS_H

#include <stddef.h>
#include <stdint.h>

#define RAMFS_MAX_FILES 256
#define RAMFS_MAX_FILENAME 256
#define RAMFS_MAX_FILESIZE 65536

typedef enum {
    RAMFS_TYPE_FILE,
    RAMFS_TYPE_DIR
} ramfs_node_type_t;

typedef struct ramfs_inode {
    uint32_t ino;
    ramfs_node_type_t type;
    char name[RAMFS_MAX_FILENAME];
    uint32_t size;
    uint32_t parent_ino;
    uint8_t data[RAMFS_MAX_FILESIZE];
} ramfs_inode_t;

typedef struct ramfs_fd {
    uint32_t ino;
    uint32_t offset;
} ramfs_fd_t;

/* Initialize RAMFS */
void ramfs_init(void);

/* File operations */
int ramfs_open(const char *path, int flags);
int ramfs_close(int fd);
int ramfs_read(int fd, void *buf, size_t count);
int ramfs_write(int fd, const void *buf, size_t count);

/* Directory operations */
int ramfs_mkdir(const char *path);
int ramfs_rmdir(const char *path);

/* File system operations */
int ramfs_unlink(const char *path);
void ramfs_ls(const char *path);
void ramfs_format(void);

/* RAMFS internals for shell/tools */
extern ramfs_inode_t ramfs_inodes[];
extern uint32_t ramfs_next_inode;

#endif
