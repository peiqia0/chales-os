#include <kernel/ramfs.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <kernel/programs.h>
#include <sys/abi.h>

ramfs_inode_t ramfs_inodes[RAMFS_MAX_FILES];

static uint8_t ramfs_pool[RAMFS_POOL_SIZE] __attribute__((aligned(16)));
static uint32_t ramfs_pool_used = 0;

static int ramfs_reserve(ramfs_inode_t *inode, uint32_t needed)
{
    if (needed <= inode->capacity) {
        return 0;
    }
    uint32_t newcap = inode->capacity ? inode->capacity * 2 : 256;
    if (newcap < needed) newcap = needed;
    if (newcap > RAMFS_MAX_FILESIZE) newcap = RAMFS_MAX_FILESIZE;
    newcap = (newcap + 15) & ~15u;

    /* Growing the newest block can happen in place. */
    if (inode->data && inode->data + inode->capacity == ramfs_pool + ramfs_pool_used) {
        if (ramfs_pool_used - inode->capacity + newcap > RAMFS_POOL_SIZE) return -1;
        ramfs_pool_used += newcap - inode->capacity;
        inode->capacity = newcap;
        return 0;
    }

    if (ramfs_pool_used + newcap > RAMFS_POOL_SIZE) {
        return -1;
    }
    uint8_t *block = ramfs_pool + ramfs_pool_used;
    ramfs_pool_used += newcap;
    if (inode->data && inode->size) {
        memcpy(block, inode->data, inode->size);
    }
    inode->data = block;
    inode->capacity = newcap;
    return 0;
}

static void ramfs_release(ramfs_inode_t *inode)
{
    if (inode->data && inode->data + inode->capacity == ramfs_pool + ramfs_pool_used) {
        ramfs_pool_used -= inode->capacity;
    }
}
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
    ramfs_add_binary("/bin/uptime", uptime_elf, uptime_elf_len);
    ramfs_add_binary("/bin/beep", beep_elf, beep_elf_len);
    ramfs_add_binary("/bin/shell", shell_elf, shell_elf_len);
    ramfs_add_binary("/bin/cowsay", cowsay_elf, cowsay_elf_len);
    ramfs_add_binary("/bin/nltest", nltest_elf, nltest_elf_len);
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
    ramfs_pool_used = 0;
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
    ramfs_file_descriptors[fd].flags = CHALES_O_RDWR; /* legacy open: always RW */

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
    if ((file_fd->flags & CHALES_O_ACCMODE) == CHALES_O_WRONLY) {
        return -1;
    }

    /* Calculate how much we can read */
    if (file_fd->offset >= inode->size) {
        return 0; /* at or past EOF (lseek can park the offset beyond it) */
    }
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

    if ((file_fd->flags & CHALES_O_ACCMODE) == CHALES_O_RDONLY) {
        return -1;
    }
    if (file_fd->flags & CHALES_O_APPEND) {
        file_fd->offset = inode->size;
    }

    /* Write what fits (a short write), and only fail when nothing does. */
    if (file_fd->offset >= RAMFS_MAX_FILESIZE) {
        return count == 0 ? 0 : -CHALES_ENOSPC;
    }
    if (file_fd->offset + count > RAMFS_MAX_FILESIZE) {
        count = RAMFS_MAX_FILESIZE - file_fd->offset;
    }
    if (count == 0) {
        return 0;
    }
    if (ramfs_reserve(inode, file_fd->offset + count) != 0) {
        /* Pool is full: fall back to whatever the current block can hold. */
        if (file_fd->offset >= inode->capacity) {
            return -CHALES_ENOSPC;
        }
        count = inode->capacity - file_fd->offset;
    }

    /* A previous lseek may have left a hole between EOF and the offset. */
    if (file_fd->offset > inode->size) {
        memset(&inode->data[inode->size], 0, file_fd->offset - inode->size);
    }

    memcpy(&inode->data[file_fd->offset], buf, count);
    file_fd->offset += count;

    if (file_fd->offset > inode->size) {
        inode->size = file_fd->offset;
    }

    return (int)count;
}

static ramfs_inode_t *ramfs_inode_by_ino(uint32_t ino)
{
    if (ino == 0) {
        return &ramfs_inodes[0]; /* root is slot 0 with ino 0 */
    }
    for (uint32_t i = 0; i < RAMFS_MAX_FILES; i++) {
        if (ramfs_inodes[i].ino == ino) {
            return &ramfs_inodes[i];
        }
    }
    return NULL;
}

static void ramfs_fill_stat(const ramfs_inode_t *inode, struct chales_stat *st)
{
    st->st_mode  = (inode->type == RAMFS_TYPE_DIR) ? (CHALES_S_IFDIR | 0755)
                                                    : (CHALES_S_IFREG | 0644);
    st->st_size  = (inode->type == RAMFS_TYPE_DIR) ? 0 : inode->size;
    st->st_ino   = inode->ino;
    st->st_nlink = 1;
}

/* open() with newlib's flag values. Returns an fd or -CHALES_E*. */
int ramfs_open2(const char *path, int oflags)
{
    if (!path) {
        return -CHALES_EFAULT;
    }

    int32_t ino = ramfs_find_inode(path);

    if (ino != -1 && (oflags & CHALES_O_CREAT) && (oflags & CHALES_O_EXCL)) {
        return -CHALES_EEXIST;
    }
    if (ino == -1) {
        if (!(oflags & CHALES_O_CREAT)) {
            return -CHALES_ENOENT;
        }

        int tmp = ramfs_open(path, 1);
        if (tmp < 0) {
            return -CHALES_ENOENT; /* missing parent directory or table full */
        }
        ramfs_close(tmp);
        ino = ramfs_find_inode(path);
        if (ino == -1) {
            return -CHALES_ENOENT;
        }
    }

    if (ramfs_inodes[ino].type == RAMFS_TYPE_DIR) {
        /* Directories can't be read/written through this fd layer yet. */
        return -CHALES_EISDIR;
    }

    int32_t fd = ramfs_find_free_fd();
    if (fd == -1) {
        return -CHALES_EMFILE;
    }

    if ((oflags & CHALES_O_TRUNC) && (oflags & CHALES_O_ACCMODE) != CHALES_O_RDONLY) {
        ramfs_inodes[ino].size = 0;
    }

    ramfs_file_descriptors[fd].ino = ramfs_inodes[ino].ino;
    ramfs_file_descriptors[fd].offset = 0;
    ramfs_file_descriptors[fd].flags = (uint32_t)oflags & (CHALES_O_ACCMODE | CHALES_O_APPEND);
    return fd;
}

int ramfs_lseek(int fd, int32_t offset, int whence)
{
    if (fd < 0 || fd >= 32 || ramfs_file_descriptors[fd].ino == 0) {
        return -CHALES_EBADF;
    }
    ramfs_fd_t *f = &ramfs_file_descriptors[fd];
    ramfs_inode_t *inode = ramfs_inode_by_ino(f->ino);
    if (!inode) {
        return -CHALES_EBADF;
    }

    int64_t base;
    switch (whence) {
        case CHALES_SEEK_SET: base = 0; break;
        case CHALES_SEEK_CUR: base = f->offset; break;
        case CHALES_SEEK_END: base = inode->size; break;
        default: return -CHALES_EINVAL;
    }

    int64_t pos = base + offset;
    if (pos < 0 || pos > RAMFS_MAX_FILESIZE) {
        return -CHALES_EINVAL;
    }
    f->offset = (uint32_t)pos;
    return (int)pos;
}

int ramfs_fstat(int fd, struct chales_stat *st)
{
    if (fd < 0 || fd >= 32 || ramfs_file_descriptors[fd].ino == 0) {
        return -CHALES_EBADF;
    }
    ramfs_inode_t *inode = ramfs_inode_by_ino(ramfs_file_descriptors[fd].ino);
    if (!inode) {
        return -CHALES_EBADF;
    }
    ramfs_fill_stat(inode, st);
    return 0;
}

int ramfs_stat(const char *path, struct chales_stat *st)
{
    int32_t ino = ramfs_find_inode(path);
    if (ino == -1) {
        return -CHALES_ENOENT;
    }
    ramfs_fill_stat(&ramfs_inodes[ino], st);
    return 0;
}

const uint8_t *ramfs_file_data(const char *path, size_t *size_out)
{
    int32_t ino = ramfs_find_inode(path);
    if (ino == -1 || ramfs_inodes[ino].type != RAMFS_TYPE_FILE) {
        return NULL;
    }
    if (size_out) {
        *size_out = ramfs_inodes[ino].size;
    }
    return ramfs_inodes[ino].data;
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

    ramfs_release(&ramfs_inodes[ino]);
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
