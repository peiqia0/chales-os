#define SYSCALL_ALIAS(real, alias_name) __typeof__(real) alias_name __attribute__((alias(#real)))

#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/abi.h>

extern int global_errno __asm__("errno");

static void set_errno(int e)
{
    global_errno = e;
    errno = e; //the macro: *__errno()
}

static inline int sys3(int num, int a, int b, int c)
{
    int ret;
    __asm__ volatile ("int $0x80"
                      : "=a"(ret)
                      : "a"(num), "b"(a), "c"(b), "d"(c)
                      : "memory");
    return ret;
}

// Kernel returns -errno on failure; convert to the C convention.
static int sys_result(int ret)
{
    if (ret < 0 && ret >= -4095) {
        set_errno(-ret);
        return -1;
    }
    return ret;
}

// process stuff

void _exit(int code)
{
    sys3(0, code, 0, 0);
    for (;;) {
        __asm__ volatile ("pause");
    }
}

pid_t getpid(void)
{
    return sys3(SYS_getpid, 0, 0, 0);
}

int kill(pid_t pid, int sig)
{
    return sys_result(sys3(SYS_kill, pid, sig, 0));
}

// No fork(): there is a single address space and no copy-on-write
pid_t fork(void)
{
    set_errno(EAGAIN);
    return -1;
}

int execve(const char *path, char *const argv[], char *const envp[])
{
    int r = sys3(21, (int)path, (int)argv, (int)envp);
    if (r < 0) {
        set_errno(ENOENT);
        return -1;
    }
    return r;
}

// Reaps the child most recently run by execve()
pid_t wait(int *status)
{
    return sys_result(sys3(SYS_wait, (int)status, 0, 0));
}

//memory stuff
void *sbrk(ptrdiff_t increment)
{
    void *prev = (void *)sys3(4, (int)increment, 0, 0);
    if (prev == (void *)-1) {
        set_errno(ENOMEM);
    }
    return prev;
}

// files stuff

int open(const char *path, int flags, ...)
{
    return sys_result(sys3(SYS_open2, (int)path, flags, 0));
}

int close(int fd)
{
    return sys_result(sys3(7, fd, 0, 0));
}

static char line_buf[256];
static int line_len = 0;
static int line_pos = 0;

static void echo(char c)
{
    sys3(9, CHALES_FD_STDOUT, (int)&c, 1);
}

static void read_line(void)
{
    line_len = 0;
    line_pos = 0;
    for (;;) {
        char c;
        int r = sys3(8, CHALES_FD_STDIN, (int)&c, 1);
        if (r == -CHALES_EAGAIN) {
            __asm__ volatile ("pause");
            continue;
        }
        if (r <= 0) {
            continue;
        }
        if (c == '\n' || c == '\r') {
            line_buf[line_len++] = '\n';
            echo('\n');
            return;
        } else if (c == '\b') {
            if (line_len > 0) {
                line_len--;
                echo('\b');
            }
        } else if (c >= 32 && c < 127 && line_len < (int)sizeof(line_buf) - 1) {
            line_buf[line_len++] = c;
            echo(c);
        }
    }
}

_READ_WRITE_RETURN_TYPE read(int fd, void *vbuf, size_t len_)
{
    char *buf = (char *)vbuf;
    int len = (int)len_;
    if (fd == CHALES_FD_STDIN) {
        if (len <= 0) {
            return 0;
        }
        if (line_pos >= line_len) {
            read_line();
        }
        int n = 0;
        while (n < len && line_pos < line_len) {
            buf[n++] = line_buf[line_pos++];
        }
        return n;
    }
    return sys_result(sys3(8, fd, (int)buf, len));
}

_READ_WRITE_RETURN_TYPE write(int fd, const void *buf, size_t len)
{
    return sys_result(sys3(9, fd, (int)buf, (int)len));
}

off_t lseek(int fd, off_t offset, int whence)
{
    return (off_t)sys_result(sys3(SYS_lseek, fd, (int)offset, whence));
}

int isatty(int fd)
{
    if (fd < 0) {
        set_errno(EBADF);
        return 0;
    }
    int r = sys3(SYS_isatty, fd, 0, 0);
    if (!r) {
        set_errno(ENOTTY);
    }
    return r;
}

static void fill_stat(struct stat *st, const struct chales_stat *k)
{
    st->st_mode = k->st_mode;
    st->st_size = k->st_size;
    st->st_ino = k->st_ino;
    st->st_nlink = k->st_nlink;
    st->st_blksize = 512;
    st->st_blocks = (k->st_size + 511) / 512;
}

int fstat(int fd, struct stat *st)
{
    struct chales_stat k;
    if (sys_result(sys3(SYS_fstat, fd, (int)&k, 0)) < 0) {
        return -1;
    }
    memset(st, 0, sizeof(*st));
    fill_stat(st, &k);
    return 0;
}

int stat(const char *path, struct stat *st)
{
    struct chales_stat k;
    if (sys_result(sys3(SYS_stat, (int)path, (int)&k, 0)) < 0) {
        return -1;
    }
    memset(st, 0, sizeof(*st));
    fill_stat(st, &k);
    return 0;
}

int unlink(const char *path)
{
    int r = sys3(11, (int)path, 0, 0);
    if (r < 0) {
        set_errno(ENOENT);
        return -1;
    }
    return 0;
}

// RAMFS has no hard links
int link(const char *old, const char *new)
{
    (void)old;
    (void)new;
    set_errno(EMLINK);
    return -1;
}

int mkdir(const char *path, mode_t mode)
{
    (void)mode;
    if (sys3(10, (int)path, 0, 0) < 0) {
        set_errno(EEXIST);
        return -1;
    }
    return 0;
}

int rmdir(const char *path)
{
    if (sys3(12, (int)path, 0, 0) < 0) {
        set_errno(ENOENT);
        return -1;
    }
    return 0;
}

// time stuff

clock_t times(struct tms *buf)
{
    struct chales_tms k;
    clock_t ticks = (clock_t)sys3(SYS_times, (int)&k, 0, 0);
    if (buf) {
        buf->tms_utime = k.tms_utime;
        buf->tms_stime = k.tms_stime;
        buf->tms_cutime = k.tms_cutime;
        buf->tms_cstime = k.tms_cstime;
    }
    return ticks;
}

int gettimeofday(struct timeval *restrict tv, void *restrict tz)
{
    (void)tz;
    struct chales_timeval k;
    if (sys_result(sys3(SYS_gettimeofday, (int)&k, 0, 0)) < 0) {
        return -1;
    }
    if (tv) {
        tv->tv_sec = k.tv_sec;
        tv->tv_usec = k.tv_usec;
    }
    return 0;
}

// names for newlib configurations that use them
SYSCALL_ALIAS(getpid, _getpid);
SYSCALL_ALIAS(kill, _kill);
SYSCALL_ALIAS(fork, _fork);
SYSCALL_ALIAS(execve, _execve);
SYSCALL_ALIAS(wait, _wait);
SYSCALL_ALIAS(sbrk, _sbrk);
SYSCALL_ALIAS(open, _open);
SYSCALL_ALIAS(close, _close);
SYSCALL_ALIAS(read, _read);
SYSCALL_ALIAS(write, _write);
SYSCALL_ALIAS(lseek, _lseek);
SYSCALL_ALIAS(isatty, _isatty);
SYSCALL_ALIAS(fstat, _fstat);
SYSCALL_ALIAS(stat, _stat);
SYSCALL_ALIAS(unlink, _unlink);
SYSCALL_ALIAS(link, _link);
SYSCALL_ALIAS(times, _times);
SYSCALL_ALIAS(gettimeofday, _gettimeofday);
