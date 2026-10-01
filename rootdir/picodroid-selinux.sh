#!/system/bin/sh
# picodroid SELinux 启动契约（用户裁定 2026-09-30）：
#   ① 部署前置：bootconfig 带 androidboot.selinux=permissive（设备 first-stage init 加载
#      策略后不 enforce，启动全程 permissive）；
#   ② 本脚本手动重载合并策略。
#   不做 ③ enforce——是否启用 SELinux 由 app 自行决定（需要时 app 调 setenforce 1）；
#   picodroid 不代做全局 enforce。
# 真实平台上 first-stage init 来自设备 ramdisk、执行时机与 enforce 姿态不受 system 控制，
# 故策略生命周期由 picodroid 自己的 rc 契约接管（本脚本=②）。
# 本脚本逐字复刻 system/core/init/selinux.cpp OpenPolicy() 的 secilc 输入集（顺序一致）：
#   plat（INIT_FORCE_DEBUGGABLE 时用 userdebug 变体，与 init 同判据；解锁检查从简）
#   + mapping/<vers>.cil（vers=/vendor/etc/selinux/plat_sepolicy_vers.txt 首行）
#   + [.compat]/system_ext/product 各自 cil+mapping+compat（存在才加，与 init 的 access(F_OK) 同判据）
#   + plat_pub_versioned + vendor_sepolicy（必须存在）
#   + odm_sepolicy（可选）+ plat_sepolicy_genfs_<v>.cil（v=genfs_labels_version.txt，可选）
#   -c 30 与 init Android.bp 的 SEPOLICY_VERSION 一致。
PATH=/system/bin:/vendor/bin
export PATH

OUT=/dev/picodroid-sepolicy.$$

die() { echo "picodroid-selinux: $*" > /dev/kmsg; exit 1; }

[ -f /sys/fs/selinux/load ] || die "selinuxfs not mounted"

VERS=$(head -n1 /vendor/etc/selinux/plat_sepolicy_vers.txt 2>/dev/null)
[ -n "$VERS" ] || die "no plat_sepolicy_vers.txt"
GENFS=$(head -n1 /vendor/etc/selinux/genfs_labels_version.txt 2>/dev/null)
[ -n "$GENFS" ] || GENFS=0

PLAT=/system/etc/selinux/plat_sepolicy.cil
if [ "$INIT_FORCE_DEBUGGABLE" = "true" ] && [ -f /system_ext/etc/selinux/userdebug_plat_sepolicy.cil ]; then
    PLAT=/system_ext/etc/selinux/userdebug_plat_sepolicy.cil
fi

EXTRA=""
add() { [ -f "$1" ] && EXTRA="$EXTRA $1"; }
add /system/etc/selinux/mapping/$VERS.compat.cil
add /system_ext/etc/selinux/system_ext_sepolicy.cil
add /system_ext/etc/selinux/mapping/$VERS.cil
add /system_ext/etc/selinux/mapping/$VERS.compat.cil
add /product/etc/selinux/product_sepolicy.cil
add /product/etc/selinux/mapping/$VERS.cil
[ -f /vendor/etc/selinux/plat_pub_versioned.cil ] || die "no plat_pub_versioned.cil"
EXTRA="$EXTRA /vendor/etc/selinux/plat_pub_versioned.cil"
[ -f /vendor/etc/selinux/vendor_sepolicy.cil ] || die "no vendor_sepolicy.cil"
EXTRA="$EXTRA /vendor/etc/selinux/vendor_sepolicy.cil"
add /odm/etc/selinux/odm_sepolicy.cil
add /system/etc/selinux/plat_sepolicy_genfs_$GENFS.cil

echo "picodroid-selinux: compiling (vers=$VERS genfs=$GENFS plat=$PLAT)" > /dev/kmsg
/system/bin/secilc "$PLAT" -m -M true -G -N -v -c 30 \
    /system/etc/selinux/mapping/$VERS.cil -o "$OUT" -f /sys/fs/selinux/null $EXTRA \
    || die "secilc failed"

# load 必须单次 write() 写入完整策略（selinuxfs 约束）：cat/重定向分块写会 EINVAL，
# 用系统的 load_policy（init 的 security_load_policy 同语义）。
/system/bin/load_policy "$OUT" || { rm -f "$OUT"; die "policy load failed"; }
rm -f "$OUT"

# 不 enforce（用户裁定：由 app 决定是否启用 SELinux；app 需要时自行 setenforce 1）
echo "picodroid-selinux: policy reloaded (enforce left to apps)" > /dev/kmsg
