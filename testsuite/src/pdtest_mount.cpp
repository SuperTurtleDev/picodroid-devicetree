// pdtest_mount —— 挂载契约判据（挂载重构终版：标准 /data + move mount + boot bind）：
//   1) /data 顶层挂载是 /userdata/boot 的 bind。判据用权威的 /proc/self/mountinfo：
//      /data 栈顶 root 字段 == "/boot" 且与 /userdata 挂载同 major:minor（同一文件系统）。
//      （实测 bind 在 /proc/mounts 的源列显示的是解析后的设备路径 /dev/block/vdaX
//      而非 bind 源路径字样——早期版本比对 "/userdata/boot" 字符串永远不成立，是
//      本测试的解析错误；叠挂检测语义不变：宿主 mount_all --late 若在 /data 顶上
//      叠挂整分区，栈顶 root 会是 "/"，判据即 FAIL。）
//   2) /userdata 为 ext4 分区挂载（move 自标准 /data 挂载的整分区）
//   3) DSU 守卫断言：DSU guest 态（ro.gsid.image_running=1）下 /userdata 挂载源
//      basename 必须 == userdata_gsi（TransformFstabForDsu 换源结果；否则挂的是
//      宿主分区，拒收）
//   4) /data 内写入在 /userdata/boot 可见（bind 双向性）
//   5) /system_ext、/product 为符号链（GSI 姿态，skip_mount.cfg 生效的旁证）
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/system_properties.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>

#define LOG(...) do { fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)

// 解析 /proc/self/mountinfo：取挂载点 == path 的栈顶条目（后行覆盖前行），
// 输出 major:minor、root 子路径、fs 类型、源设备。任一出参可传 nullptr 忽略。
static bool mountinfo_top(const char* path, char* dev_out, size_t dev_len,
                          char* fs_out, size_t fs_len,
                          char* root_out, size_t root_len, char* id_out, size_t id_len) {
    FILE* f = fopen("/proc/self/mountinfo", "r");
    if (!f) return false;
    char line[2048];
    bool found = false;
    while (fgets(line, sizeof(line), f)) {
        int mid = 0, parent = 0;
        char id[64] = {0}, root[512] = {0}, mnt[512] = {0};
        if (sscanf(line, "%d %d %63s %511s %511s", &mid, &parent, id, root, mnt) < 5) continue;
        if (strcmp(mnt, path) != 0) continue;
        // 行内 '-' 之后的 "fstype source super_options"
        char* dash = strstr(line, " - ");
        if (!dash) continue;
        char fs[64] = {0}, src[256] = {0};
        if (sscanf(dash + 3, "%63s %255s", fs, src) < 2) continue;
        if (dev_out) snprintf(dev_out, dev_len, "%s", src);
        if (fs_out) snprintf(fs_out, fs_len, "%s", fs);
        if (root_out) snprintf(root_out, root_len, "%s", root);
        if (id_out) snprintf(id_out, id_len, "%s", id);
        found = true;  // 取最后一条（挂载栈顶）
    }
    fclose(f);
    return found;
}

// 兼容旧签名：仅取源设备与 fs（DSU 守卫用源设备 basename）
static bool mountinfo_top(const char* path, char* dev_out, size_t dev_len, char* fs_out, size_t fs_len) {
    return mountinfo_top(path, dev_out, dev_len, fs_out, fs_len, nullptr, 0, nullptr, 0);
}

static const char* base_name(const char* path) {
    const char* s = strrchr(path, '/');
    return s ? s + 1 : path;
}

int main() {
    bool ok = true;
    char data_dev[256] = {0}, data_fs[64] = {0};
    char ud_dev[256] = {0}, ud_fs[64] = {0};

    // 1) /data 顶层是 /userdata/boot 的 bind：mountinfo 栈顶 root=="/boot" 且与
    //    /userdata 同 major:minor（同文件系统，见文件头注释）
    char data_root[512] = {0}, data_id[64] = {0}, ud_id[64] = {0}, ud_root[512] = {0};
    bool d_ok = mountinfo_top("/data", data_dev, sizeof(data_dev), data_fs, sizeof(data_fs),
                              data_root, sizeof(data_root), data_id, sizeof(data_id));
    bool u_ok = mountinfo_top("/userdata", ud_dev, sizeof(ud_dev), ud_fs, sizeof(ud_fs),
                              ud_root, sizeof(ud_root), ud_id, sizeof(ud_id));
    LOG("mount[/data]: top dev=%s root=%s (%s)", data_dev, data_root, data_fs);
    LOG("mount[/userdata]: top dev=%s root=%s (%s)", ud_dev, ud_root, ud_fs);
    if (!d_ok) {
        LOG("FAIL: /data 未挂载"); ok = false;
    } else if (strcmp(data_root, "/boot") != 0) {
        LOG("FAIL: /data 栈顶 root=%s（应为 /boot；宿主叠挂未清或契约未生效）", data_root); ok = false;
    } else if (!u_ok || strcmp(data_id, ud_id) != 0) {
        LOG("FAIL: /data(%s) 与 /userdata(%s) 非同一文件系统（bind 关系不成立）", data_id, ud_id); ok = false;
    }

    // 2) /userdata 为 ext4
    if (!u_ok) {
        LOG("FAIL: /userdata 未挂载"); ok = false;
    } else if (strcmp(ud_fs, "ext4") != 0) {
        LOG("FAIL: /userdata 是 %s（应为 ext4）", ud_fs); ok = false;
    }

    // 3) DSU 守卫：DSU guest 态 /userdata 源 basename 必须 == userdata_gsi
    char gsi[PROP_VALUE_MAX] = {0};
    __system_property_get("ro.gsid.image_running", gsi);
    if (gsi[0] == '1') {
        const char* ud_base = base_name(ud_dev);
        if (strcmp(ud_base, "userdata_gsi") != 0) {
            LOG("FAIL: DSU guest 但 /userdata 源是 %s（应为 userdata_gsi；宿主分区！拒收）", ud_dev);
            ok = false;
        } else {
            LOG("dsu guard: /userdata 源 userdata_gsi 正确");
        }
    }

    // 4) bind 双向性：/data 写入在 /userdata/boot 可见、经 bind 侧删除双向同步
    struct stat st;
    if (stat("/userdata/boot", &st) != 0) {
        if (errno == EACCES) {
            LOG("SKIP: /userdata/boot 不可访问（非 root；用 adb root 后复测）");
        } else {
            LOG("FAIL: /userdata/boot 目录不存在"); ok = false;
        }
    } else {
        const char* probe = "/userdata/boot/.pdtest_mount_probe";
        FILE* f = fopen(probe, "w");
        if (!f) {
            if (errno == EACCES) LOG("SKIP: /userdata/boot 写探针（非 root；用 adb root 后复测）");
            else { LOG("FAIL: /userdata/boot 不可写"); ok = false; }
        } else {
            fputs("probe", f); fclose(f);
            if (access("/data/.pdtest_mount_probe", F_OK) != 0) {
                LOG("FAIL: /userdata/boot 与 /data 不同源（bind 未生效）"); ok = false;
            } else {
                unlink("/data/.pdtest_mount_probe");  // 经 bind 侧删除，双向可见
                if (access(probe, F_OK) == 0) { unlink(probe); LOG("FAIL: bind 双向性异常"); ok = false; }
            }
        }
    }

    // 5) GSI 姿态旁证：/system_ext、/product 应为符号链（独立判据，不与上文捆绑）
    bool posture_ok = lstat("/system_ext", &st) == 0 && S_ISLNK(st.st_mode) &&
                      lstat("/product", &st) == 0 && S_ISLNK(st.st_mode);
    if (!posture_ok) { LOG("FAIL: /system_ext 或 /product 非符号链（GSI 姿态旁证失败）"); ok = false; }

    if (ok) LOG("PASS: mount contract (标准 /data + move mount + /userdata/boot bind + GSI 姿态)");
    return ok ? 0 : 10;
}
