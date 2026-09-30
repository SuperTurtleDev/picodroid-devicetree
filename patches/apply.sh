#!/bin/bash
# picodroid 平台补丁应用脚本（双轨之一；另一轨为各项目本地提交）
# 用法：在 /work/aosp 下执行 bash device/anland/picodroid/patches/apply.sh
set -e
AOSP=${1:-/work/aosp}
for p in "$AOSP"/device/anland/picodroid/patches/*.patch; do
    proj=$(grep -m1 '^--- a/' "$p" | sed 's|^--- a/||' | cut -d/ -f1-2)
    echo "applying $(basename "$p") -> $proj"
    git -C "$AOSP/$proj" apply "$p"
done
