#!/bin/bash
# 同步构建出的 pdtest 二进制进 app 契约 bin/<uname -m>/。
# 测试二进制不进镜像（用户裁定 2026-09-30），由契约携带、adb push 到机器；
# start.sh 以 exec bin/$(uname -m)/pdtest_* 引用。
# 用法：tools_sync_test_bins.sh <product-out-arch-dir> <build_arch>
#   例：tools_sync_test_bins.sh /work/aosp/out/target/product/anland/picodroid/x86_64 x86_64
# build_arch: x86_64|arm64 → 落目录名 x86_64|aarch64（uname -m 口径）
set -e
OUT_BIN="$1/system/bin"
ARCH_IN="$2"
case "$ARCH_IN" in
    x86_64) UNAME_M=x86_64 ;;
    arm64)   UNAME_M=aarch64 ;;
    *) echo "unknown arch: $ARCH_IN (expect x86_64|arm64)" >&2; exit 1 ;;
esac
[ -d "$OUT_BIN" ] || { echo "no such dir: $OUT_BIN" >&2; exit 1; }
HERE=$(cd "$(dirname "$0")" && pwd)

# 固定映射清单（app 契约 → 模块产物名）；不用通配遍历，清单即契约
sync_one() {
    app="$1"; bin="$2"
    dst="$HERE/apps/$app/bin/$UNAME_M"
    mkdir -p "$dst"
    cp -f "$OUT_BIN/$bin" "$dst/$bin"
    chmod 755 "$dst/$bin"
    echo "apps/$app/bin/$UNAME_M/$bin"
}
sync_one dns_1             pdtest_dns
sync_one hal_1             pdtest_hal
sync_one pd_audio_1        pdtest_audio
sync_one pd_audio_appuid_1 pdtest_audio_appuid
sync_one pd_audio_ll_1     pdtest_audio_ll
sync_one pd_bluetooth_1    pdtest_bluetooth
sync_one pd_camera_1       pdtest_camera
sync_one pd_encode_1       pdtest_encode
sync_one pd_media_1        pdtest_media
sync_one pd_mount_1        pdtest_mount
sync_one pd_sensor_1       pdtest_sensor
