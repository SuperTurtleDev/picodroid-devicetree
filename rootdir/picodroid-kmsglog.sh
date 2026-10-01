#!/system/bin/sh
# picodroid-kmsglog —— 真机 DSU debug（2026-10-01 用户裁定）：
# dmesg 同步落盘。`cat /dev/kmsg` 打开即全量回放环形缓冲、随后阻塞跟随新消息，
# 追加写 /metadata/picodroid-dmesg.log（DSU guest 中 /metadata 为真机 metadata 分区，
# gsid-bootstrap 已挂载；回原厂系统后 su 可读）。
# /metadata 10s 内不可写（未挂/ro）→ 静默退出（kmsg 本身仍在，仅丢持久化）。
LOG=/metadata/picodroid-dmesg.log

i=0
while [ $i -lt 10 ] && ! touch "$LOG" 2>/dev/null; do
    sleep 1
    i=$((i + 1))
done
[ -w "$LOG" ] || exit 0

# 尺寸护栏：超 8MB 只留尾部 4MB（debug 用途，多启动累积时防无限增长）
SZ=$(stat -c %s "$LOG" 2>/dev/null || echo 0)
if [ "$SZ" -gt 8388608 ]; then
    tail -c 4194304 "$LOG" > "$LOG.trunc" 2>/dev/null && mv "$LOG.trunc" "$LOG"
fi

echo "==== picodroid-kmsglog start $(date '+%m-%d %H:%M:%S') uname=$(uname -r) ====" >> "$LOG"
# 周期 sync：/metadata 为 f2fs，强制断电会回滚到上个检查点——实测丢过整轮日志
( while true; do sleep 2; sync; done ) &
cat /dev/kmsg >> "$LOG" 2>/dev/null
