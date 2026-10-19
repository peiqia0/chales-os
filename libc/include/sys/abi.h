#ifndef _SYS_CHALES_ABI_H
#define _SYS_CHALES_ABI_H



#include <stdint.h>


#define SYS_open2          22 // (path, newlib O_* flags, mode)          */
#define SYS_lseek          23 // (fd, offset, whence)                    */
#define SYS_fstat          24 /* (fd, struct chales_stat *)              */
#define SYS_stat           25 /* (path, struct chales_stat *)            */
#define SYS_isatty         26 /* (fd)                                    */
#define SYS_getpid         27 /* ()                                      */
#define SYS_kill           28 /* (pid, sig)                              */
#define SYS_times          29 /* (struct chales_tms *)                   */
#define SYS_gettimeofday   30 /* (struct chales_timeval *)               */
#define SYS_wait           31 /* (int *status) -> pid of finished child  */

/* File descriptors 0, 1 and 2 are the console. RAMFS files start at 3. */
#define CHALES_FD_STDIN    0
#define CHALES_FD_STDOUT   1
#define CHALES_FD_STDERR   2
#define CHALES_FD_FILE_BASE 3

/* open() flags: these are newlib's values (sys/_default_fcntl.h). */
#define CHALES_O_ACCMODE   0x0003
#define CHALES_O_RDONLY    0x0000
#define CHALES_O_WRONLY    0x0001
#define CHALES_O_RDWR      0x0002
#define CHALES_O_APPEND    0x0008
#define CHALES_O_CREAT     0x0200
#define CHALES_O_TRUNC     0x0400
#define CHALES_O_EXCL      0x0800

/* st_mode bits (same as POSIX / newlib). */
#define CHALES_S_IFMT      0170000
#define CHALES_S_IFCHR     0020000
#define CHALES_S_IFDIR     0040000
#define CHALES_S_IFREG     0100000

/* Errno values, identical to newlib's <sys/errno.h>. */
#define CHALES_ENOENT   2
#define CHALES_ESRCH    3
#define CHALES_EBADF    9
#define CHALES_ECHILD   10
#define CHALES_EAGAIN   11
#define CHALES_ENOMEM   12
#define CHALES_EFAULT   14
#define CHALES_EEXIST   17
#define CHALES_ENOTDIR  20
#define CHALES_EISDIR   21
#define CHALES_EINVAL   22
#define CHALES_EMFILE   24
#define CHALES_ENOSPC   28
#define CHALES_ESPIPE   29
#define CHALES_EMLINK   31
#define CHALES_ENOSYS   88

/* lseek whence */
#define CHALES_SEEK_SET 0
#define CHALES_SEEK_CUR 1
#define CHALES_SEEK_END 2

struct chales_stat {
    uint32_t st_mode;
    uint32_t st_size;
    uint32_t st_ino;
    uint32_t st_nlink;
};

/* Times are in PIT ticks; the PIT runs at 1000 Hz. */
#define CHALES_TICKS_PER_SEC 1000

struct chales_tms {
    uint32_t tms_utime;
    uint32_t tms_stime;
    uint32_t tms_cutime;
    uint32_t tms_cstime;
};

struct chales_timeval {
    uint32_t tv_sec;
    uint32_t tv_usec;
};

#endif
