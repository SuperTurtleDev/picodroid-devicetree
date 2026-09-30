#!/system/bin/sh
# pdtest_all —— picodroid testsuite 汇总入口。
# M1 调试期：adb shell pdtest_all（/data 未契约化前产物落 PD_OUT，默认 /userdata/boot/tests，
# 不可写时回退 /data/local/tmp —— 与 PLAN §4 契约的绑定路径一致，M3 落地后自动归位）。
LOGDIR="${PD_LOGDIR:-/userdata/boot/logs}"
[ -d "$LOGDIR" ] || LOGDIR=/data/local/tmp
OUTDIR="${PD_OUT:-/userdata/boot/tests}"
if ! mkdir -p "$OUTDIR" 2>/dev/null; then OUTDIR=/data/local/tmp; fi
export PD_OUT="$OUTDIR"

LOG="$LOGDIR/pdtest_all.log"
: > "$LOG"

pass=0; fail=0; list=""
for t in pdtest_audio pdtest_sensor pdtest_media pdtest_camera pdtest_bluetooth pdtest_mount pdtest_encode pdtest_audio_ll pdtest_audio_appuid pdtest_hal pdtest_dns; do
    if [ ! -x "/system/bin/$t" ]; then
        echo "SKIP  $t (binary missing)" | tee -a "$LOG"; continue
    fi
    echo "==== RUN  $t ====" | tee -a "$LOG"
    if "/system/bin/$t" >>"$LOG" 2>&1; then
        echo "PASS  $t" | tee -a "$LOG"; pass=$((pass+1))
    else
        echo "FAIL  $t" | tee -a "$LOG"; fail=$((fail+1)); list="$list $t"
    fi
done
echo "==== SUMMARY: pass=$pass fail=$fail ====" | tee -a "$LOG"
[ -n "$list" ] && echo "failed:$list" | tee -a "$LOG"
echo "log: $LOG  artifacts: $OUTDIR"
[ "$fail" -eq 0 ]
