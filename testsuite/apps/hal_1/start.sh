#!/system/bin/sh
# picodroid app: hal v1 —— vendor HAL 面测试（见 testsuite/）
export PATH=/system/bin:/vendor/bin
export PD_OUT=/userdata/boot/tests/hal
export PD_LOGDIR=/userdata/boot/logs
mkdir -p "$PD_OUT" 2>/dev/null || true
# 平台契约（非测试逻辑）：picodroid 无 bpfloader，bpf.progs_loaded 恒空会让
# vendor.health-cuttlefish 主线程死等该属性、不响应任何 binder 事务（实测
# dumpsys --pid 挂起、服务唯一线程卡 futex）。无 BPF 是 picodroid 既定形态，
# 在此置位解锁：服务自行重启后进入 epoll 主循环正常对外服务。
[ "$(getprop bpf.progs_loaded)" = "1" ] || setprop bpf.progs_loaded 1
exec /data/local/tmp/pdtest_hal
