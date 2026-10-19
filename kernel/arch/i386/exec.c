#include <kernel/exec.h>
#include <kernel/elf.h>
#include <kernel/ramfs.h>
#include <kernel/sched.h>
#include <string.h>
#include <stdio.h>

#include "paging.h"


#define EXEC_POOL_SIZE (512 * 1024)
static uint8_t exec_pool[EXEC_POOL_SIZE] __attribute__((aligned(16)));
static uint32_t exec_pool_used = 0;

#define EXEC_MAX_ARGS 32
#define EXEC_ARG_BYTES 4096
static char kargs[EXEC_ARG_BYTES];
static char *kargv[EXEC_MAX_ARGS + 1];
static char *kenvp[EXEC_MAX_ARGS + 1];

extern uint8_t *user_brk;

static uint32_t align16(uint32_t v) { return (v + 15u) & ~15u; }

static int copy_strings(char **src, char **out, uint32_t *used)
{
    int n = 0;
    if (src != NULL) {
        for (; src[n] != NULL; n++) {
            if (n >= EXEC_MAX_ARGS) return -1;
            const char *s = src[n];
            out[n] = &kargs[*used];
            for (;;) {
                if (*used >= EXEC_ARG_BYTES) return -1;
                char c = *s++;
                kargs[(*used)++] = c;
                if (c == '\0') break;
            }
        }
    }
    out[n] = NULL;
    return n;
}

static void frame_restore(uint32_t off, uint32_t stack_len,
                          uint32_t img_base, uint32_t img_len)
{
    if (stack_len) {
        memcpy((void *)(uintptr_t)(EXEC_STACK_TOP - EXEC_STACK_SIZE),
               exec_pool + off, stack_len);
    }
    if (img_len) {
        memcpy((void *)(uintptr_t)img_base, exec_pool + off + stack_len, img_len);
    }
}

void exec_restore(task_t *child)
{
    if (!child->has_frame) return;
    frame_restore(child->frame_off, child->frame_stack_len,
                  child->frame_img_base, child->frame_img_len);
    user_brk = child->frame_brk;
    exec_pool_used = child->frame_off;
    child->has_frame = 0;
}

int exec_run(const char *path, char **argv, char **envp)
{
    task_t *parent = current_task;
    if (path == NULL) return -1;

    size_t size = 0;
    const uint8_t *image = ramfs_file_data(path, &size);
    if (image == NULL || size == 0) return -1;

    uint32_t lo, hi;
    if (elf_extent(image, size, &lo, &hi) != 0) return -1;

    uint32_t used = 0;
    if (copy_strings(argv, kargv, &used) < 0 ||
        copy_strings(envp, kenvp, &used) < 0) {
        printk("[exec] argv/envp too large\n");
        return -1;
    }

    uint32_t stack_len = 0, img_base = 0, img_len = 0;
    if (parent->elf_hi != 0) {
        stack_len = EXEC_STACK_SIZE;
        img_base = lo > parent->elf_lo ? lo : parent->elf_lo;
        uint32_t img_end = hi < parent->elf_hi ? hi : parent->elf_hi;
        img_len = img_end > img_base ? img_end - img_base : 0;
    }

    uint32_t frame_off = exec_pool_used;
    uint32_t frame_len = align16(stack_len) + align16(img_len);
    if (frame_off + frame_len > EXEC_POOL_SIZE) {
        printk("[exec] out of memory to save the caller\n");
        return -1;
    }

    if (stack_len) {
        memcpy(exec_pool + frame_off,
               (void *)(uintptr_t)(EXEC_STACK_TOP - EXEC_STACK_SIZE), stack_len);
    }
    if (img_len) {
        memcpy(exec_pool + frame_off + stack_len, (void *)(uintptr_t)img_base, img_len);
    }
    exec_pool_used = frame_off + frame_len;

    // Failure from here on must put the caller's memory back.
#define EXEC_FAIL() do { \
        frame_restore(frame_off, stack_len, img_base, img_len); \
        exec_pool_used = frame_off; \
        return -1; \
    } while (0)

    paging_map_user_range(EXEC_STACK_TOP - EXEC_STACK_SIZE, EXEC_STACK_TOP);
    uint32_t sp = elf_build_user_stack(EXEC_STACK_TOP - EXEC_STACK_SIZE,
                                       EXEC_STACK_TOP, kargv, kenvp);
    if (sp == 0) EXEC_FAIL();

    uint32_t entry;
    if (elf_load(image, size, &entry) != 0) EXEC_FAIL();

    task_t *child = sched_task_new(entry, sp);
    if (child == NULL) EXEC_FAIL();
#undef EXEC_FAIL

    child->parent = parent;
    child->elf_lo = lo;
    child->elf_hi = hi;
    child->has_frame = 1;
    child->frame_off = frame_off;
    child->frame_stack_len = stack_len;
    child->frame_img_base = img_base;
    child->frame_img_len = img_len;
    child->frame_brk = user_brk;

    parent->state = TASK_BLOCKED;
    schedule();   // returns once the child has exited and woken us
    return 0;
}
