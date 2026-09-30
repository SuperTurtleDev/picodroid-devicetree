# anland_picodroid_arm64 —— 真机 GSI 产品（与 x86_64 同步维护）
$(call inherit-product, device/anland/picodroid/picodroid_common.mk)

# 64 位单架构（PLAN §3；真机若发现 32 位 vendor HAL 再开 multilib）
TARGET_SUPPORTS_32_BIT_APPS := false
TARGET_SUPPORTS_64_BIT_APPS := true
TARGET_SUPPORTS_OMX_SERVICE := false

PRODUCT_ENFORCE_ARTIFACT_PATH_REQUIREMENTS := relaxed
MODULE_BUILD_FROM_SOURCE := true

PRODUCT_NAME := anland_picodroid_arm64
PRODUCT_DEVICE := anland/picodroid/arm64
PRODUCT_BRAND := Android
PRODUCT_MODEL := picodroid on arm64

PRODUCT_SOONG_DEFINED_SYSTEM_IMAGE := picodroid_system
USE_SOONG_DEFINED_SYSTEM_IMAGE := true

# arm64 专属：hwasan 运行库由镜像模块自暂存（文件名带 aarch64-android 后缀，
# make 侧无法同名镜像，走 filelistdiff allowlist——见 TRIM-LEDGER）。