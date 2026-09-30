#!/system/bin/sh
# picodroid app: pd_camera v1 —— NDK 功能测试（见 testsuite/）
export PATH=/system/bin:/vendor/bin
export PD_OUT=/userdata/boot/tests/pd_camera
export PD_LOGDIR=/userdata/boot/logs
mkdir -p "$PD_OUT" 2>/dev/null || true
exec /system/bin/pdtest_camera
