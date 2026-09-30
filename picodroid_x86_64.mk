# anland_picodroid_x86_64 —— Cuttlefish 日常迭代产品
$(call inherit-product, device/anland/picodroid/picodroid_common.mk)

# 64 位单架构（PLAN §3；无 zygote/ART，无 32 位需求）
TARGET_SUPPORTS_32_BIT_APPS := false
TARGET_SUPPORTS_64_BIT_APPS := true
TARGET_SUPPORTS_OMX_SERVICE := false

PRODUCT_ENFORCE_ARTIFACT_PATH_REQUIREMENTS := relaxed
MODULE_BUILD_FROM_SOURCE := true

PRODUCT_NAME := anland_picodroid_x86_64
PRODUCT_DEVICE := anland/picodroid/x86_64
PRODUCT_BRAND := Android
PRODUCT_MODEL := picodroid on x86_64

PRODUCT_SOONG_DEFINED_SYSTEM_IMAGE := picodroid_system
USE_SOONG_DEFINED_SYSTEM_IMAGE := true
