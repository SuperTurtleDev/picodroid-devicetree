#!/system/bin/sh
# picodroid-launcher —— PLAN §7 应用契约启动器。
# 契约：
#   /data/app/<id>_<version>/   id 可含下划线；version=最后一个 _ 后的数字（无后缀视为 0）
#                               同 id 多版本取 sort -V 最高
#   start.sh                    cwd=应用目录、root、PATH=/system/bin:/vendor/bin
#                               stdout/stderr → /userdata/boot/logs/<id>.log
#   depends                     每行一个依赖；# 注释与空行忽略
#     <appid>                   app 依赖（拓扑序 + 环检测）
#     service::<init服务名>     运行前置的 init 服务：ctl.start（幂等）拉起并轮询
#                               init.svc.<名>=running（超时 15s、间隔 0.2s；restarting 视为
#                               继续等待；超时仍不 running → 该 app skipped 记 launcher.log）。
#                               不计入 app 拓扑环检测。
# 拓扑：Kahn 迭代；任一依赖 fail/skipped → 该 app skipped；成环整环 skipped 并告警。
# 状态：setprop picodroid.apps.<id>={ok|fail|skipped}；决策链写 launcher.log。

PATH=/system/bin:/vendor/bin
export PATH

APPROOT=/data/app
LOGROOT=/userdata/boot/logs
LAUNCH_LOG=$LOGROOT/launcher.log
T=/data/local/tmp/.pdlaunch
rm -rf "$T"; mkdir -p "$T/st" "$T/dep" "$T/svc"

# ---- 属性垫片（宿主单测用）：真机走 setprop/getprop；宿主（无 setprop）以 $T/prop 文件
# 模拟属性区。ctl.start 交给可执行钩子 $PD_HOST_SVC（入参 env：PD_SVCNAME=服务名、
# PD_PROPDIR=属性目录）模拟 init 拉起；无钩子 = 服务不存在（init.svc.* 恒空 → 超时）。
# 真机不会进入 else 分支。
if command -v setprop >/dev/null 2>&1; then
    pd_setprop() { setprop "$@"; }
    pd_getprop() { getprop "$1" 2>/dev/null; }
else
    pd_getprop() { cat "$T/prop/$1" 2>/dev/null; }
    pd_setprop() {
        case "$1" in
            ctl.start)
                if [ -n "$PD_HOST_SVC" ] && [ -x "$PD_HOST_SVC" ]; then
                    PD_PROPDIR="$T/prop" PD_SVCNAME="$2" "$PD_HOST_SVC" &
                fi ;;
            ctl.stop) ;;
            *) mkdir -p "$T/prop"; printf '%s\n' "$2" > "$T/prop/$1" ;;
        esac
    }
fi

log() { echo "launcher: $*" >> "$LAUNCH_LOG" 2>/dev/null; echo "launcher: $*" > /dev/kmsg; }

[ -d "$APPROOT" ] || { log "no $APPROOT; nothing to launch"; exit 0; }
mkdir -p "$LOGROOT" 2>/dev/null

# ---- 扫描：id -> 最高版本目录 ----
for d in "$APPROOT"/*; do
    [ -d "$d" ] || continue
    [ -f "$d/start.sh" ] || { log "skip $(basename "$d"): no start.sh"; continue; }
    b=$(basename "$d")
    id=${b%_*}; ver=${b##*_}
    case "$ver" in *[!0-9]*) id=$b; ver=0;; esac
    echo "$id $ver $d" >> "$T/apps.raw"
done
[ -f "$T/apps.raw" ] || { log "no apps found"; exit 0; }
# 每 id 取 sort -V 最高版本（升序遍历、同 id 后行覆盖前行 → 留最高）
sort -k1,1 -k2,2V "$T/apps.raw" | awk '{a[$1]=$0} END {for (i in a) print a[i]}' > "$T/apps"
rm -f "$T/apps.raw"

ids=$(awk '{print $1}' "$T/apps")
dir_of() { awk -v id="$1" '$1==id{print $3; exit}' "$T/apps"; }

# ---- 依赖表（app 依赖与 service:: 服务依赖分拆；后者不进拓扑）----
for id in $ids; do
    d=$(dir_of "$id")
    if [ -f "$d/depends" ]; then
        grep -v '#' "$d/depends" 2>/dev/null | sed 's/[[:space:]]*$//' | grep -v '^$' > "$T/dep/$id.all"
        grep -v '^service::' "$T/dep/$id.all" > "$T/dep/$id"
        sed -n 's/^service:://p' "$T/dep/$id.all" > "$T/svc/$id"
        rm -f "$T/dep/$id.all"
    fi
done

st() { cat "$T/st/$1" 2>/dev/null || echo pending; }

# ---- service::<name> 处理：ctl.start（幂等）后轮询 init.svc.<name>=running ----
SVC_WAIT_TIMEOUT=${PD_SVC_WAIT:-15}   # 秒；宿主单测可用 PD_SVC_WAIT 缩短
SVC_WAIT_INTERVAL=0.2
case "$SVC_WAIT_TIMEOUT" in ''|*[!0-9.]*) SVC_WAIT_TIMEOUT=15;; esac

ensure_service() {
    svc=$1
    [ "$(pd_getprop "init.svc.$svc")" = "running" ] && return 0
    pd_setprop ctl.start "$svc"
    polls=$(awk "BEGIN{printf \"%d\", int(($SVC_WAIT_TIMEOUT / $SVC_WAIT_INTERVAL) + 0.5)}")
    i=0
    while [ "$i" -lt "$polls" ]; do
        sleep "$SVC_WAIT_INTERVAL"
        i=$((i + 1))
        [ "$(pd_getprop "init.svc.$svc")" = "running" ] && return 0
    done
    log "service $svc not running after ${SVC_WAIT_TIMEOUT}s (state=$(pd_getprop "init.svc.$svc"))"
    return 1
}

# ---- Kahn 迭代 ----
while :; do
    progress=0
    for id in $ids; do
        [ "$(st "$id")" = "pending" ] || continue
        unmet=0; bad=0; total=0
        if [ -s "$T/dep/$id" ]; then
            while IFS= read -r dep; do
                total=$((total+1))
                case "$(st "$dep")" in
                    ok) ;;
                    fail|skipped) bad=$((bad+1)); unmet=$((unmet+1)) ;;
                    *) unmet=$((unmet+1)) ;;
                esac
            done < "$T/dep/$id"
        fi
        if [ "$unmet" -eq 0 ]; then
            # service:: 依赖先行（app 依赖全 ok 后、执行前）；任一超时不 running → skipped
            svcfail=""
            if [ -s "$T/svc/$id" ]; then
                while IFS= read -r svc; do
                    ensure_service "$svc" || svcfail="$svcfail $svc"
                done < "$T/svc/$id"
            fi
            if [ -n "$svcfail" ]; then
                echo skipped > "$T/st/$id"; pd_setprop picodroid.apps."$id" skipped
                log "$id skipped (service dependency not running:$svcfail)"
                progress=1
                continue
            fi
            d=$(dir_of "$id")
            log "run $id ($d)"
            rc=0
            ( cd "$d" && sh ./start.sh ) >> "$LOGROOT/$id.log" 2>&1 || rc=$?
            if [ "$rc" -eq 0 ]; then
                echo ok > "$T/st/$id"; pd_setprop picodroid.apps."$id" ok; log "$id ok"
            else
                echo fail > "$T/st/$id"; pd_setprop picodroid.apps."$id" fail; log "$id fail rc=$rc"
            fi
            progress=1
        elif [ "$unmet" -eq "$total" ] && [ "$bad" -gt 0 ]; then
            echo skipped > "$T/st/$id"; pd_setprop picodroid.apps."$id" skipped
            log "$id skipped (dependency failed/skipped)"
            progress=1
        fi
    done
    # 全部非 pending 或本轮无进展则收工
    done_all=1
    for id in $ids; do [ "$(st "$id")" = "pending" ] && done_all=0; done
    [ "$done_all" = "1" ] && break
    [ "$progress" = "0" ] && break
done

# ---- 剩余 pending = 环 ----
for id in $ids; do
    [ "$(st "$id")" = "pending" ] || continue
    echo skipped > "$T/st/$id"; pd_setprop picodroid.apps."$id" skipped
    log "$id skipped (dependency cycle)"
done

rm -rf "$T"
log "done"
exit 0
