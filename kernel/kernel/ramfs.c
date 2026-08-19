#include <kernel/ramfs.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <kernel/hello_elf.h>

ramfs_inode_t ramfs_inodes[RAMFS_MAX_FILES];
static ramfs_fd_t ramfs_file_descriptors[32];
uint32_t ramfs_next_inode = 1;
static uint32_t ramfs_root_ino = 0;
static void ramfs_add_binary(const char *path, const void *data, size_t size);

static void ramfs_populate(void)
{
    int fd;

    // Make a programs directory
    ramfs_mkdir("/bin");
    ramfs_add_binary("/bin/hello", hello_elf, hello_elf_len);
}

static void ramfs_add_binary(const char *path, const void *data, size_t size)
{
    int fd = ramfs_open(path, 1);

    if (fd >= 0) {
        ramfs_write(fd, data, size);
        ramfs_close(fd);
    }
}

/* Initialize RAMFS */
void ramfs_init(void)
{
    memset(ramfs_inodes, 0, sizeof(ramfs_inodes));
    memset(ramfs_file_descriptors, 0, sizeof(ramfs_file_descriptors));

    /* Create root directory */
    ramfs_inodes[0].ino = 0;
    ramfs_inodes[0].type = RAMFS_TYPE_DIR;
    strcpy(ramfs_inodes[0].name, "/");
    ramfs_inodes[0].size = 0;
    ramfs_inodes[0].parent_ino = 0;

    ramfs_root_ino = 0;
    ramfs_next_inode = 1;
    ramfs_populate();
    printk("[ramfs] initialized (root inode: %lu)\n", (unsigned long)ramfs_root_ino);
}

/* Resolves a directory path to its ino number (not array index — this is
 * what ramfs_inode_t.parent_ino stores). "" and "/" both mean root. Walks
 * one path segment at a time, requiring each intermediate segment to
 * actually be a directory. Returns -1 if any segment doesn't exist. */
static int32_t ramfs_resolve_dir(const char *dirpath)
{
    uint32_t current_ino = ramfs_root_ino;

    if (!dirpath || dirpath[0] == '\0') {
        return (int32_t)current_ino;
    }

    const char *p = dirpath;
    if (*p == '/') {
        p++;
    }

    while (*p) {
        const char *seg_start = p;
        while (*p && *p != '/') {
            p++;
        }
        size_t seg_len = (size_t)(p - seg_start);

        if (seg_len == 0) {
            if (*p == '/') p++;
            continue;
        }

        int32_t found = -1;
        for (uint32_t i = 0; i < RAMFS_MAX_FILES; i++) {
            if (ramfs_inodes[i].ino != 0 &&
                ramfs_inodes[i].parent_ino == current_ino &&
                ramfs_inodes[i].type == RAMFS_TYPE_DIR &&
                strncmp(ramfs_inodes[i].name, seg_start, seg_len) == 0 &&
                ramfs_inodes[i].name[seg_len] == '\0') {
                found = (int32_t)i;
                break;
            }
        }

        if (found == -1) {
            return -1;
        }

        current_ino = ramfs_inodes[found].ino;

        if (*p == '/') {
            p++;
        }
    }

    return (int32_t)current_ino;
}

static const char *ramfs_split_path(const char *path, char *dirbuf, size_t dirbuf_size)
{
    const char *slash = NULL;
    for (const char *p = path; *p; p++) {
        if (*p == '/') {
            slash = p;
        }
    }

    if (!slash) {
        dirbuf[0] = '\0';
        return path;
    }

    size_t dir_len = (size_t)(slash - path);
    if (dir_len >= dirbuf_size) {
        dir_len = dirbuf_size - 1;
    }
    memcpy(dirbuf, path, dir_len);
    dirbuf[dir_len] = '\0';

    return slash + 1;
}

/* Find an inode by path */
static int32_t ramfs_find_inode(const char *path)
{
    if (!path || strlen(path) == 0)
        return (int32_t)ramfs_root_ino;

    if (path[0] == '/' && strlen(path) == 1)
        return (int32_t)ramfs_root_ino;

    char dirbuf[RAMFS_MAX_FILENAME];
    const char *name = ramfs_split_path(path, dirbuf, sizeof(dirbuf));

    int32_t parent_ino = ramfs_resolve_dir(dirbuf);
    if (parent_ino == -1) {
        return -1;
    }

    for (uint32_t i = 0; i < RAMFS_MAX_FILES; i++) {
        if (ramfs_inodes[i].ino != 0 &&
            ramfs_inodes[i].parent_ino == (uint32_t)parent_ino &&
            strcmp(ramfs_inodes[i].name, name) == 0) {
            return (int32_t)i;
        }
    }

    return -1;
}

/* Find next free inode slot */
static int32_t ramfs_find_free_inode(void)
{
    for (uint32_t i = 1; i < RAMFS_MAX_FILES; i++) {
        if (ramfs_inodes[i].ino == 0) {
            return (int32_t)i;
        }
    }
    return -1;
}

/* Find next free file descriptor */
static int32_t ramfs_find_free_fd(void)
{
    for (int i = 0; i < 32; i++) {
        if (ramfs_file_descriptors[i].ino == 0) {
            return i;
        }
    }
    return -1;
}

/* Open a file */
int ramfs_open(const char *path, int flags)
{
    int32_t ino = ramfs_find_inode(path);

    /* File doesn't exist */
    if (ino == -1) {
        if (flags & 1) { /* O_CREAT */
            ino = ramfs_find_free_inode();
            if (ino == -1) {
                return -1; /* No free inodes */
            }

            char dirbuf[RAMFS_MAX_FILENAME];
            const char *name = ramfs_split_path(path, dirbuf, sizeof(dirbuf));

            int32_t parent_ino = ramfs_resolve_dir(dirbuf);
            if (parent_ino == -1) {
                return -1; /* parent directory doesn't exist */
            }

            ramfs_inodes[ino].ino = ramfs_next_inode++;
            ramfs_inodes[ino].type = RAMFS_TYPE_FILE;
            strcpy(ramfs_inodes[ino].name, name);
            ramfs_inodes[ino].size = 0;
            ramfs_inodes[ino].parent_ino = (uint32_t)parent_ino;
        } else {
            return -1; /* File not found */
        }
    }

    if (ramfs_inodes[ino].type != RAMFS_TYPE_FILE) {
        return -1; /* Not a file */
    }

    /* Find free file descriptor */
    int32_t fd = ramfs_find_free_fd();
    if (fd == -1) {
        return -1; /* No free file descriptors */
    }

    ramfs_file_descriptors[fd].ino = ramfs_inodes[ino].ino;
    ramfs_file_descriptors[fd].offset = 0;

    return fd;
}

/* Close a file */
int ramfs_close(int fd)
{
    if (fd < 0 || fd >= 32) {
        return -1;
    }

    if (ramfs_file_descriptors[fd].ino == 0) {
        return -1;
    }

    memset(&ramfs_file_descriptors[fd], 0, sizeof(ramfs_fd_t));
    return 0;
}

/* Read from a file */
int ramfs_read(int fd, void *buf, size_t count)
{
    if (fd < 0 || fd >= 32 || !buf) {
        return -1;
    }

    ramfs_fd_t *file_fd = &ramfs_file_descriptors[fd];
    if (file_fd->ino == 0) {
        return -1;
    }

    /* Find inode by ino */
    ramfs_inode_t *inode = NULL;
    for (uint32_t i = 0; i < RAMFS_MAX_FILES; i++) {
        if (ramfs_inodes[i].ino == file_fd->ino) {
            inode = &ramfs_inodes[i];
            break;
        }
    }

    if (!inode || inode->type != RAMFS_TYPE_FILE) {
        return -1;
    }

    /* Calculate how much we can read */
    size_t available = inode->size - file_fd->offset;
    size_t to_read = count < available ? count : available;

    if (to_read > 0) {
        memcpy(buf, &inode->data[file_fd->offset], to_read);
        file_fd->offset += to_read;
    }

    return (int)to_read;
}

/* Write to a file */
int ramfs_write(int fd, const void *buf, size_t count)
{
    if (fd < 0 || fd >= 32 || !buf) {
        return -1;
    }

    ramfs_fd_t *file_fd = &ramfs_file_descriptors[fd];
    if (file_fd->ino == 0) {
        return -1;
    }

    /* Find inode by ino */
    ramfs_inode_t *inode = NULL;
    for (uint32_t i = 0; i < RAMFS_MAX_FILES; i++) {
        if (ramfs_inodes[i].ino == file_fd->ino) {
            inode = &ramfs_inodes[i];
            break;
        }
    }

    if (!inode || inode->type != RAMFS_TYPE_FILE) {
        return -1;
    }

    /* Check if we have space */
    if (file_fd->offset + count > RAMFS_MAX_FILESIZE) {
        return -1; /* File too large */
    }

    memcpy(&inode->data[file_fd->offset], buf, count);
    file_fd->offset += count;

    if (file_fd->offset > inode->size) {
        inode->size = file_fd->offset;
    }

    return (int)count;
}

/* Create a directory */
int ramfs_mkdir(const char *path)
{
    if (!path || strlen(path) == 0) {
        return -1;
    }

    int32_t ino = ramfs_find_inode(path);
    if (ino != -1) {
        return -1; /* Already exists */
    }

    ino = ramfs_find_free_inode();
    if (ino == -1) {
        return -1; /* No free inodes */
    }

    char dirbuf[RAMFS_MAX_FILENAME];
    const char *name = ramfs_split_path(path, dirbuf, sizeof(dirbuf));

    int32_t parent_ino = ramfs_resolve_dir(dirbuf);
    if (parent_ino == -1) {
        return -1; /* parent directory doesn't exist */
    }

    ramfs_inodes[ino].ino = ramfs_next_inode++;
    ramfs_inodes[ino].type = RAMFS_TYPE_DIR;
    strcpy(ramfs_inodes[ino].name, name);
    ramfs_inodes[ino].size = 0;
    ramfs_inodes[ino].parent_ino = (uint32_t)parent_ino;

    return 0;
}

/* Remove a directory */
int ramfs_rmdir(const char *path)
{
    int32_t ino = ramfs_find_inode(path);
    if (ino == -1 || ramfs_inodes[ino].type != RAMFS_TYPE_DIR) {
        return -1;
    }

    memset(&ramfs_inodes[ino], 0, sizeof(ramfs_inode_t));
    return 0;
}

/* Delete a file */
int ramfs_unlink(const char *path)
{
    int32_t ino = ramfs_find_inode(path);
    if (ino == -1 || ramfs_inodes[ino].type != RAMFS_TYPE_FILE) {
        return -1;
    }

    memset(&ramfs_inodes[ino], 0, sizeof(ramfs_inode_t));
    return 0;
}

/* List directory contents */
void ramfs_ls(const char *path)
{
    int32_t parent_ino = ramfs_find_inode(path);
    if (parent_ino == -1) {
        printk("[ramfs] directory not found\n");
        return;
    }

    if (ramfs_inodes[parent_ino].type != RAMFS_TYPE_DIR) {
        printk("[ramfs] not a directory\n");
        return;
    }

    for (uint32_t i = 0; i < RAMFS_MAX_FILES; i++) {
        if (ramfs_inodes[i].ino != 0 && ramfs_inodes[i].parent_ino == ramfs_inodes[parent_ino].ino) {
            const char *type = ramfs_inodes[i].type == RAMFS_TYPE_DIR ? "DIR" : "FILE";
            if (type == "DIR") {
                printk("  %s/\n", ramfs_inodes[i].name);
                continue;
            } else if (type == "FILE") {
                printk("  %s (%lu bytes)\n", ramfs_inodes[i].name, (unsigned long)ramfs_inodes[i].size);
            }
        }
    }
}

/* Format RAMFS (clear all data) */
void ramfs_format(void)
{
    ramfs_init();
    printk("[ramfs] formatted\n");
}
