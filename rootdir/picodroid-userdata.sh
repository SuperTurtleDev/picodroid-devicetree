#!/system/bin/sh
# picodroid-userdata —— 挂载重构终版（2026-09-30 用户多轮裁定定稿）。
# 由 init.picodroid.userdata.rc 以三个同步服务（exec_start，apexd-bootstrap 同款模式）
# 分别在 early-fs（provision → mount_all → hook）与 late-fs（hook2）调用。
#
# 架构一句话：fstab 用单条目标准形态挂 /data（DSU 换源走系统原生
# TransformFstabForDsu，fstab.cpp:559-571/676-690）；挂载后 hook 用 move mount 把
# /data 移到 /userdata，再建 /userdata/boot 子目录 bind 回 /data；hook2 清宿主叠挂
# 后校验拓扑并置 picodroid.data.ready=1。
#
# 子命令：
#   provision —— mount_all 前置：ext4 有效性预检（见下"永不格式化合法 ext4"原则）。
#   hook      —— mount_all 后置：/data → move → /userdata，/userdata/boot → bind → /data。
#   hook2     —— late-fs：清宿主 mount_all --late 叠挂，校验拓扑，置 ready。
#
# 最终拓扑：
#   /userdata : ext4（整块 userdata 分区，move 自标准 /data 挂载）
#   /data     : /userdata/boot 的 bind（picodroid 专数据区）
#   /userdata/boot 之外的区域预留给后续 Linux rootfs 装载。
#
# boot 子目录命名理由：userdata 后续装载 Linux rootfs，boot（容器启动环境驱动）作
# picodroid 专数据区，避免容器应用错误清理。
#
# 核心原则——合法 ext4 盘永不格式化（用户补充裁定，优先级高于一切判据）：
#   只要目标盘已是合法 ext4（偏移 1080 魔数 0xEF53），一律不格式化、不触碰内容——
#   不管盘上有没有 /boot 目录、有没有任何 picodroid 标识文件（明确禁止依赖标记文件
#   做清零判断）。盘上 /userdata/boot 目录缺失时由 hook 的 mkdir 幂等重建，目录缺失
#   不是格式化理由，数据其余部分原样保留。理由：数据安全优先——boot 目录可重建，
#   盘数据不可。格式化只发生在"非 ext4 盘"（含空白/wiped/f2fs 等）且非 DSU 态；
#   格式化也只清零首 4096 字节交标准 wiped→formattable→fs_mgr_do_format 的
#   mke2fs+e2fsdroid（fs_mgr.cpp:1707-1747），本脚本无任何 mke2fs。
#
# DSU 态（gsid guest）零接触：gsid 安装期已清零 userdata_gsi 首 4K
# （system/gsid/partition_installer.cpp:301-325 Format），空白盘由 mount_all 的
# formattable 标准链自动重建，本脚本不写任何 DSU 设备。
#
# 为什么 hook 必须是 shell 服务：init rc 的 mount builtin 旗标表无 move
# （builtins.cpp:465-483），toybox mount 支持 -o move（toys/lsb/mount.c:115）——
# move mount 只能在 shell 服务里做，这正是 hook 存在的理由。
#
# 源码依据（android17 本树）：DSU 指示文件 booted/init.cpp 置 ro.gsid.image_running；
# dm 名 userdata_gsi 全链路同名（TRIM-LEDGER「挂载重构」节汇总）。
MAPPER=/dev/block/mapper
BYNAME=/dev/block/by-name
DSU_DIR=/metadata/gsi/dsu
LOG="picodroid-mount"

log() { echo "$LOG: $*" >> /dev/kmsg 2>/dev/null || echo "$LOG: $*"; }

# DSU 判定（provision/hook 共用）：ro.gsid.image_running=1 或 booted 指示文件存在。
DSU=0
[ "$(getprop ro.gsid.image_running 2>/dev/null)" = "1" ] && DSU=1
[ -e "$DSU_DIR/booted" ] && DSU=1

# /proc/mounts 顶层（最后一条）指定挂载点的源设备
top_src() { awk -v m="$1" '$2==m{s=$1} END{print s}' /proc/mounts; }

# DSU 数据源栈解析（真机实证修订 2026-10-01）：OPPO 真机在 gsid 的 userdata_gsi
# 之外再包 OEM dm 层（实测栈顶 dm-17），/proc/mounts 显示内核名而非 userdata_gsi
# 符号名——旧"basename==userdata_gsi"判据在真机误杀（FATAL 后契约全断）。
# 改为语义判定：从源设备的 /sys/block/<dev> 沿 slaves 展开整栈，收集设备名与
# dm 名（/sys/block/<dev>/dm/name）：栈含 dm 名 userdata_gsi → gsid 虚拟设备
# （无论外面包几层）；栈含 by-name/userdata 实设备 → 宿主分区（拒绝）。
stack_walk() {  # $1=/sys/block/<dev>；输出栈内设备名与 dm 名
    local d="$1" s n
    [ -d "$d" ] || return 0
    echo "$(basename "$d")"
    n=$(cat "$d/dm/name" 2>/dev/null)
    [ -n "$n" ] && echo "name:$n"
    for s in "$d"/slaves/*; do
        [ -e "$s" ] || continue
        stack_walk "/sys/block/$(basename "$s")"
    done
}
# 源设备（/proc/mounts 形态：/dev/block/dm-N 或 mapper 名）→ /sys/block 目录
src_sysdir() {
    local b l
    b=$(basename "$1")
    case "$b" in
        dm-*) echo "/sys/block/$b" ;;
        *) l=$(readlink -f "/dev/block/mapper/$b" 2>/dev/null)
           [ -n "$l" ] && echo "/sys/block/$(basename "$l")" ;;
    esac
}

CMD=${1:-}
case "$CMD" in

provision)
    # DSU 态零接触：gsid 已清零 userdata_gsi 首 4K，交给标准 formattable 链。
    if [ "$DSU" = "1" ]; then
        log "provision: DSU guest; zero-touch (gsid zeroed userdata_gsi at install)"
        exit 0
    fi

    # 设备按 fstab 同源解析（仅探测不改挂载语义）：mapper/userdata 优先，
    # by-name/userdata 兜底（CF 为物理分区，仅 by-name 存在）。
    DEV=""
    for d in "$MAPPER/userdata" "$BYNAME/userdata"; do
        [ -b "$d" ] && { DEV="$d"; break; }
    done
    if [ -z "$DEV" ]; then
        log "FATAL: provision: no userdata device ($MAPPER/userdata, $BYNAME/userdata)"
        exit 1
    fi
    # 合法 ext4（偏移 1080 魔数 0xEF53，小端字节序 53 ef）→ 永不触碰。
    MAGIC=$(dd if="$DEV" bs=1 skip=1080 count=2 2>/dev/null | od -An -tx1 | tr -d ' \n')
    if [ "$MAGIC" = "53ef" ]; then
        log "provision: $DEV is valid ext4; data preserved"
        exit 0
    fi
    # 非 ext4：AUDIT + 清零首 4096 字节 + sync → 标准 wiped→formattable→
    # fs_mgr_do_format（mke2fs+e2fsdroid）接管；脚本自身无任何 mke2fs。
    SIZE=$(blockdev --getsize64 "$DEV" 2>/dev/null || echo unknown)
    log "AUDIT: provision wipe device=$DEV size=$SIZE (not ext4; zero first 4096 bytes only, standard formattable flow will mkfs; data loss by contract)"
    dd if=/dev/zero of="$DEV" bs=4096 count=1 conv=notrunc 2>/dev/null \
        || { log "FATAL: provision: zeroing $DEV failed"; exit 1; }
    sync
    exit 0
    ;;

hook)
    # 等 mount_all 完成：/proc/mounts 出现挂载点 /data（10s 超时 FATAL）。
    i=0
    while [ "$i" -lt 10 ] && [ -z "$(top_src /data)" ]; do
        sleep 1
        i=$((i + 1))
    done
    [ -n "$(top_src /data)" ] || { log "FATAL: hook: /data not mounted within 10s"; exit 1; }
    # move /data → /userdata：bind 到新挂载点后卸掉原挂载点（语义等价 mount --move；
    # 本机 init 环境下 MS_MOVE 不可用，bind+umount 是等价且已实证的做法）。
    mount -o bind /data /userdata \
        || { log "FATAL: hook: bind /data /userdata failed"; exit 1; }
    # DSU 守卫（用户既定 + 真机实证修订 2026-10-01）：DSU 态数据源栈必须含 gsid
    # 虚拟设备 userdata_gsi（真机外层另有 OEM dm 包裹，栈顶名非 userdata_gsi），
    # 且不得含真机 by-name/userdata 实设备（宿主分区保护）。
    if [ "$DSU" = "1" ]; then
        SRC=$(top_src /userdata)
        STACK=$(stack_walk "$(src_sysdir "$SRC")")
        log "hook: DSU source stack: $(echo "$STACK" | tr '\n' ' ')"
        REAL=$(basename "$(readlink -f "$BYNAME/userdata")")
        if echo "$STACK" | grep -q "^name:userdata_gsi$"; then
            : # gsid 虚拟设备链（可含 OEM dm 包裹）——放行
        elif echo "$STACK" | grep -qx "$REAL"; then
            log "FATAL: hook: DSU guest but source stack hits host partition $REAL; refusing"
            exit 1
        else
            log "FATAL: hook: DSU guest but no userdata_gsi in source stack ($SRC); refusing"
            exit 1
        fi
    fi
    umount /data \
        || { log "FATAL: hook: umount /data failed"; exit 1; }
    # picodroid 数据区 = 分区内 boot 子目录（幂等重建目录，不动其余数据）。
    mkdir -p /userdata/boot \
        || { log "FATAL: hook: mkdir /userdata/boot failed"; exit 1; }
    mount -o bind /userdata/boot /data \
        || { log "FATAL: hook: bind /userdata/boot /data failed"; exit 1; }
    log "hook: done (data->userdata, boot->data)"

    exit 0
    ;;

hook2)
    # 宿主 mount_all --late 及其 umount_retry 会把 /data 的叠挂清栈/重挂；
    # 清空 /data 后重新 bind 即可。
    i=0
    while [ -n "$(top_src /data)" ] && [ "$i" -lt 8 ]; do
        umount /data 2>/dev/null
        i=$((i + 1))
    done
    mount -o bind /userdata/boot /data \
        || { log "FATAL: hook2: bind /userdata/boot /data failed"; exit 1; }
    setprop picodroid.data.ready 1
    log "hook2: ready"

    exit 0
    ;;

*)
    log "FATAL: unknown command '${CMD}' (usage: picodroid-userdata provision|hook|hook2)"
    exit 1
    ;;
esac
