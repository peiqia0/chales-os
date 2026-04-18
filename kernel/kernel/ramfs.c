#include <kernel/ramfs.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

ramfs_inode_t ramfs_inodes[RAMFS_MAX_FILES];
static ramfs_fd_t ramfs_file_descriptors[32];
uint32_t ramfs_next_inode = 1;
static uint32_t ramfs_root_ino = 0;

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
    
    printf("RAMFS: initialized (root inode: %lu)\n", (unsigned long)ramfs_root_ino);
}

/* Find an inode by path */
static int32_t ramfs_find_inode(const char *path)
{
    if (!path || strlen(path) == 0)
        return ramfs_root_ino;
    
    if (path[0] == '/' && strlen(path) == 1)
        return ramfs_root_ino;
    
    /* Simple path parsing: skip leading '/' and find matching name */
    const char *name_start = path[0] == '/' ? path + 1 : path;
    
    for (uint32_t i = 0; i < RAMFS_MAX_FILES; i++) {
        if (ramfs_inodes[i].ino != 0) {
            if (strcmp(ramfs_inodes[i].name, name_start) == 0) {
                return (int32_t)i;
            }
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
            
            const char *name = strrchr(path, '/');
            name = name ? name + 1 : path;
            
            ramfs_inodes[ino].ino = ramfs_next_inode++;
            ramfs_inodes[ino].type = RAMFS_TYPE_FILE;
            strcpy(ramfs_inodes[ino].name, name);
            ramfs_inodes[ino].size = 0;
            ramfs_inodes[ino].parent_ino = ramfs_root_ino;
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
    
    const char *name = strrchr(path, '/');
    name = name ? name + 1 : path;
    
    ramfs_inodes[ino].ino = ramfs_next_inode++;
    ramfs_inodes[ino].type = RAMFS_TYPE_DIR;
    strcpy(ramfs_inodes[ino].name, name);
    ramfs_inodes[ino].size = 0;
    ramfs_inodes[ino].parent_ino = ramfs_root_ino;
    
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
        printf("ramfs: directory not found\n");
        return;
    }
    
    if (ramfs_inodes[parent_ino].type != RAMFS_TYPE_DIR) {
        printf("ramfs: not a directory\n");
        return;
    }
    
    for (uint32_t i = 0; i < RAMFS_MAX_FILES; i++) {
        if (ramfs_inodes[i].ino != 0 && ramfs_inodes[i].parent_ino == ramfs_inodes[parent_ino].ino) {
            const char *type = ramfs_inodes[i].type == RAMFS_TYPE_DIR ? "DIR" : "FILE";
            printf("  [%s] %s (%lu bytes)\n", type, ramfs_inodes[i].name, (unsigned long)ramfs_inodes[i].size);
        }
    }
}

/* Format RAMFS (clear all data) */
void ramfs_format(void)
{
    ramfs_init();
    printf("RAMFS: formatted\n");
}
