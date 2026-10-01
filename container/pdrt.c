// pdrt —— picodroid base container runtime（2026-10-01）。
// 形态：契约 app 二进制（apps/container_ubuntu_1/bin/<arch>/pdrt），随 app push。
// 能力（最小集，按用户裁定：PID NS 容器）：
//   pdrt run <rootfs> [command...] —— CLONE_NEWPID + CLONE_NEWNS，
//   子进程内 mount 新 /proc（PID NS 配套）、bind /dev、pivot_root 进 rootfs、exec。
//   无 command 时默认 /bin/sh -i。父进程 wait 并透传退出码。
// 设计要点：
//   - CLONE_NEWNS 必须（pivot_root 与 proc 重挂不能泄漏宿主 mount 表）；
//   - 其余 namespace（UTS/IPC/NET）共享——容器与宿主同网栈，继承 picodroid 的
//     DNS/路由面（dns_1 契约已建）；
//   - rootfs 由契约 start.sh 准备（ubuntu-base tarball 解到 /data/containers/ubuntu）。
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef pivot_root
#define pivot_root(new_root, put_old) syscall(SYS_pivot_root, new_root, put_old)
#endif

static int child_main(void* arg) {
    char** argv = (char**)arg;
    const char* rootfs = argv[0];

    // mount 表隔离已在 clone 完成；MS_REC 传播 private，避免后续挂载外泄
    mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL);

    // 新 PID NS 的 /proc
    char procbuf[512];
    snprintf(procbuf, sizeof(procbuf), "%s/proc", rootfs);
    mkdir(procbuf, 0755);
    mount("proc", procbuf, "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV, NULL);

    // /dev 复用宿主（PID-NS-only 形态；设备节点由宿主 devtmpfs 提供）
    char devbuf[512];
    snprintf(devbuf, sizeof(devbuf), "%s/dev", rootfs);
    mkdir(devbuf, 0755);
    mount("/dev", devbuf, NULL, MS_BIND | MS_REC, NULL);
    // /sys 同享（HAL 调试可见面）
    char sysbuf[512];
    snprintf(sysbuf, sizeof(sysbuf), "%s/sys", rootfs);
    mkdir(sysbuf, 0755);
    mount("/sys", sysbuf, NULL, MS_BIND | MS_REC, NULL);

    // pivot_root: rootfs 成为新根，旧根挂到 rootfs/.pd_oldroot
    char old[512];
    snprintf(old, sizeof(old), "%s/.pd_oldroot", rootfs);
    mkdir(old, 0755);
    if (pivot_root(rootfs, old) != 0) {
        fprintf(stderr, "pdrt: pivot_root(%s): %s\n", rootfs, strerror(errno));
        return 126;
    }
    chdir("/");
    // 旧根卸载（分离后再 umount，避免引用保持）
    mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL);
    umount2("/.pd_oldroot", MNT_DETACH);
    rmdir("/.pd_oldroot");

    setenv("container", "pdrt", 1);

    char** cmd = argv + 1;
    if (cmd[0] == NULL) {
        static char* def[] = {(char*)"/bin/sh", (char*)"-i", NULL};
        cmd = def;
    }
    execvp(cmd[0], cmd);
    fprintf(stderr, "pdrt: exec %s: %s\n", cmd[0], strerror(errno));
    return 127;
}

int main(int argc, char** argv) {
    if (argc < 3 || strcmp(argv[1], "run") != 0) {
        fprintf(stderr, "usage: pdrt run <rootfs> [command...]\n");
        return 2;
    }

    // Ubuntu rootfs 的动态链接器依赖 ld cache——容器内 resolver 由 rootfs 自带
    const size_t stack_size = 1024 * 1024;
    char* stack = malloc(stack_size);
    if (!stack) {
        fprintf(stderr, "pdrt: oom\n");
        return 1;
    }

    int flags = SIGCHLD | CLONE_NEWPID | CLONE_NEWNS;
    pid_t pid = clone(child_main, stack + stack_size, flags, argv + 2);
    if (pid < 0) {
        fprintf(stderr, "pdrt: clone(NEWPID|NEWNS): %s\n", strerror(errno));
        return 125;
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        fprintf(stderr, "pdrt: waitpid: %s\n", strerror(errno));
        return 1;
    }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 1;
}
