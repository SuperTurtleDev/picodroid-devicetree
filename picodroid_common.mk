# picodroid 公共产品配置
# 复制自 build/make/target/product/gsi_release.mk 后按台账删减（TRIM-LEDGER.md），
# 并附 soong 镜像 deps 的 make 侧镜像（file_list_diff 一致性由构建校验）。

# picodroid_system 镜像模块门控（android_gsi 同款）：本产品置位后 bp 侧 enabled。
# 其它产品（aosp_cf_* 等）构建时 soong 仍解析本仓 Android.bp，不门控会因
# image 模块"仅含通用模块"约束炸 bootstrap。
$(call add_soong_config_namespace,picodroid)
$(call soong_config_set_bool,picodroid,building,true)

PRODUCT_ARTIFACT_PATH_REQUIREMENT_ALLOWED_LIST += \
    system/etc/init/config \
    system/product/% \
    system/system_ext/%

# GSI 应支持最新平台特性（gsi_release 原文）
PRODUCT_SHIPPING_API_LEVEL := $(PLATFORM_SDK_VERSION)

# 动态分区（便于混入 Cuttlefish）+ 动态分区大小
PRODUCT_USE_DYNAMIC_PARTITIONS := true
PRODUCT_USE_DYNAMIC_PARTITION_SIZE := true

# 16KB 页显式声明（gsi_release 原文）
PRODUCT_NO_BIONIC_PAGE_SIZE_MACRO := true
PRODUCT_MAX_PAGE_SIZE_SUPPORTED := 16384

# apex 机制承重（android17：adbd/i18n/runtime 仅存于 apex；详见 TRIM-LEDGER.md 决策记录）
PRODUCT_SYSTEM_PROPERTIES += ro.apex.updatable=true

# 只构建 system 镜像（gsi_release 原文）
PRODUCT_BUILD_CACHE_IMAGE := false
PRODUCT_BUILD_DEBUG_BOOT_IMAGE := false
PRODUCT_BUILD_DEBUG_VENDOR_BOOT_IMAGE := false
PRODUCT_BUILD_USERDATA_IMAGE := false
PRODUCT_BUILD_VENDOR_IMAGE := false
PRODUCT_BUILD_SUPER_PARTITION := false
PRODUCT_BUILD_SUPER_EMPTY_IMAGE := false
PRODUCT_BUILD_SYSTEM_DLKM_IMAGE := false
PRODUCT_EXPORT_BOOT_IMAGE_TO_DIST := true

PRODUCT_PRODUCT_PROPERTIES += \
    ro.crypto.metadata_init_delete_all_keys.enabled=false \
    debug.codec2.bqpool_dealloc_after_stop=1

# GSI 分区姿态：skip_mount.cfg + GSI 版本探测 rc 垫片（沿用 GSI 原模块；gsi_release 原文）
# + HAL 存活配套
PRODUCT_PACKAGES += \
    gsi_skip_mount.cfg \
    init.gsi.rc \
    init.vndk-nodef.rc \
    hwservicemanager \
    android.hidl.allocator@1.0-service \
    android.hidl.memory@1.0-impl

# ===== soong 镜像 deps 的 make 侧镜像（与 Android.bp 保持锁步；差异由 file_list_diff 报告）=====
PRODUCT_PACKAGES += \
    mediametrics \
    init.picodroid.userdata.rc \
    picodroid-userdata \
    picodroid-launcher \
    picodroid-selinux \
    picodroid-kmsglog \
    fstab.picodroid \
    com.android.tethering \
    com.android.vndk.v31 \
    com.android.vndk.v32 \
    com.android.vndk.v33 \
    com.android.vndk.v34 \
    com.android.os.statsd \
    com.android.configinfrastructure \
    pmg_daemon \
    abx \
    aconfigd-system \
    aflags \
    aoad \
    android.system.suspend-service \
    apexd \
    apexd.mainline_patch_level_2 \
    atrace \
    audioserver \
    blkid \
    bootstat \
    bpfloader \
    bugreport \
    bugreportz \
    cameraserver \
    casefolding_remover \
    cgroups.json \
    cmd \
    debuggerd \
    dmctl \
    dmesgd \
    dpm \
    dump.erofs \
    dumpstate \
    dumpsys \
    e2fsck \
    evemu-record \
    etc_hosts \
    flags_health_check \
    fs_config_dirs_system \
    fs_config_files_system \
    fsck.erofs \
    fsck.f2fs \
    fsck_msdos \
    gpu_counter_producer \
    gpuservice \
    group_system \
    gsi_tool \
    gsid \
    heapprofd \
    hid \
    hidservice \
    idc_data \
    init.environ.rc-soong \
    init.usb.configfs.rc \
    init.usb.rc \
    ip \
    iptables \
    kcmdlinectrl \
    kcmdlinemodprobe \
    keychars_data \
    keylayout_data \
    llkd \
    logcat \
    logd \
    lpdump \
    lshal \
    make_f2fs \
    media_profiles_V1_0.dtd \
    mediacodec.policy \
    mediacodeclist_generator \
    mediaextractor \
    misctrl \
    mke2fs \
    mkfs.erofs \
    mm_daemon \
    mm_daemon_setup \
    mtectrl \
    netutils-wrapper-1.0 \
    notice_xml_system \
    passwd_system \
    pbtombstone \
    perfetto \
    ping \
    ping6 \
    pintool \
    prefetch \
    prng_seeder \
    public.libraries.android.txt \
    recovery-persist \
    recovery-refresh \
    resize2fs \
    rss_hwm_reset \
    run-as \
    schedtest \
    secdiscard \
    sensorservice \
    service \
    servicemanager \
    sgdisk \
    snapuserd \
    storaged \
    system_manifest.xml \
    task_profiles.json \
    tc \
    tombstoned \
    traced \
    traced_probes \
    tune2fs \
    uinput \
    uncrypt \
    update_engine \
    usbd \
    virtual_camera \
    watchdogd \
    wifi.rc \
    com.android.neuralnetworks \
    com.android.uprobestats \
    com.android.npumanager \
    com.android.runtime \
    adbd_system_api \
    build_flag_system \
    charger_res_images \
    framework_compatibility_matrix.device.xml \
    hwservicemanager_compat_symlink_module \
    hyph-data \
    init_system \
    llndk.libraries.txt \
    perfetto-extras \
    sanitizer.libraries.txt \
    selinux_policy_system_soong \
    shell_and_utilities_system \
    system-build.prop \
    system_compatibility_matrix.xml \
    drmserver \
    mediaserver \
    android.system.virtualizationcommon-ndk \
    android.system.virtualizationservice-ndk \
    libgsi \
    libandroid_native_denylist \
    libnpumanager \
    android.hardware.secure_element@1.0 \
    boringssl_self_test \
    heapprofd_client \
    libETC1 \
    libFFTEm \
    libOpenMAXAL \
    libOpenSLES \
    libaaudio \
    libandroid \
    libartpalette-system \
    libaudio-resampler \
    libaudiohal \
    libaudiopolicyengineconfigurable \
    libbinder \
    libbinder_ndk \
    libbinder_rpc_unstable \
    libcamera2ndk \
    libcgrouprc \
    libclang_rt.asan \
    libcompiler_rt \
    libcutils \
    libdmabufheap \
    libdrm \
    libdrmframework \
    libfdtrack \
    libfilterfw \
    libfilterpack_imageproc \
    libfwdlockengine \
    libhardware \
    libhardware_legacy \
    libhidltransport \
    libhwbinder \
    libinput \
    libiprouteutil \
    libjnigraphics \
    libjpeg \
    liblog \
    liblogwrap \
    liblz4 \
    libmedia \
    libmediandk \
    libminui \
    libmtp \
    libnetd_client \
    libnetlink \
    libnetutils \
    libneuralnetworks_packageinfo \
    libnl \
    libpdfium \
    libpolicy-subsystem \
    libpower \
    libpowermanager \
    libprotobuf-cpp-full \
    libradio_metadata \
    libsensorservice \
    libsfplugin_ccodec \
    libskia \
    libsonic \
    libsonivox \
    libsoundpool \
    libspeexresampler \
    libsqlite \
    libstagefright \
    libstagefright_foundation \
    libstagefright_omx \
    libstdc++ \
    libsysutils \
    libui \
    libusbhost \
    libutils \
    libvendorsupport \
    libvulkan \
    libwilhelm \
    linker \
    com.android.adbd \
    com.android.i18n \
    com.android.tzdata \
    com.android.resolv \
    com.android.media \
    com.android.media.swcodec \
    build_flag_system_ext \
    fs_config_dirs_system_ext \
    fs_config_files_system_ext \
    group_system_ext \
    passwd_system_ext \
    selinux_policy_system_ext \
    system_ext_manifest.xml \
    system_ext-build.prop \
    charger \
    build_flag_product \
    fs_config_dirs_product \
    fs_config_files_product \
    group_product \
    passwd_product \
    product_compatibility_matrix.xml \
    product_manifest.xml \
    selinux_policy_product \
    product-build.prop \
    gsi_skip_mount.cfg \
    hwservicemanager \
    android.hidl.allocator@1.0-service \
    android.hidl.memory@1.0-impl

PRODUCT_PACKAGES_DEBUG += \
    alloctop \
    adevice_fingerprint \
    arping \
    avbctl \
    bootctl \
    dmuserd \
    evemu-record \
    idlcli \
    init-debug.rc \
    iotop \
    iperf3 \
    iw \
    logpersist.start \
    logtagd.rc \
    lpmodify \
    ot-cli-ftd \
    ot-ctl \
    overlay_remounter \
    procrank \
    profcollectctl \
    profcollectd \
    record_binder \
    sanitizer-status \
    servicedispatcher \
    showmap \
    sqlite3 \
    ss \
    start_with_lockagent \
    strace \
    su \
    tinycap \
    tinyhostless \
    tinymix \
    tinypcminfo \
    tinyplay \
    tracepath \
    tracepath6 \
    traceroute6 \
    unwind_info \
    unwind_reg_info \
    unwind_symbols \
    update_engine_client
