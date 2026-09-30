PRODUCT_MAKEFILES := \
    anland_picodroid_arm64:$(LOCAL_DIR)/picodroid_arm64.mk \
    anland_picodroid_x86_64:$(LOCAL_DIR)/picodroid_x86_64.mk

COMMON_LUNCH_CHOICES := \
    anland_picodroid_arm64-trunk_staging-userdebug \
    anland_picodroid_x86_64-trunk_staging-userdebug
