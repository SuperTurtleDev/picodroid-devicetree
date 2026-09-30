# picodroid 裁剪台账（GSI → picodroid）

> 生成于 2026-09-29T10:37:03+00:00
> 纪律：复制 build/make/target/product/{generic,gsi}/Android.bp 四个镜像 defaults 后仅做下列删除；
> 每处删除可追溯到类目。凡不在本表者一律原文保留（有节制裁剪）。

## Java应用（87）
`AppSearchAiSealConfig`, `BackupRestoreConfirmation`, `BasicDreams`, `BlockedNumberProvider`, `BluetoothMidiService`, `BookmarkProvider`, `Browser2`, `BuiltInPrintService`, `CalendarProvider`, `CallLogBackup`, `Camera2`, `CameraExtensionsProxy`, `CaptivePortalLogin`, `CarrierConfig`, `CarrierDefaultApp`, `CellBroadcastLegacyApp`, `CertInstaller`, `CompanionDeviceManager`, `ContactsPicker`, `ContactsProvider`, `CredentialManager`, `CrossDeviceSync`, `DeviceAsWebcam`, `DeviceDiagnostics`, `Dialer`, `DocumentsUI`, `DownloadProvider`, `DownloadProviderUi`, `DynamicSystemInstallationService`, `E2eeContactKeysProvider`, `EasterEgg`, `ExtShared`, `ExternalStorageProvider`, `EyeDropper`, `FusedLocation`, `HTMLViewer`, `InputDevices`, `IntentResolver`, `KeyChain`, `LatinIME`, `Launcher3QuickStep`, `LiveWallpapersPicker`, `LocalTransport`, `ManagedProvisioning`, `MmsService`, `ModuleMetadata`, `MtpService`, `MusicFX`, `NetworkStack`, `ONS`, `PacProcessor`, `PackageInstaller`, `PartnerBookmarksProvider`, `PrintRecommendationService`, `PrintSpooler`, `PrivateSpace`, `Provision`, `ProxyHandler`, `SatelliteClient`, `SecureElement`, `Settings`, `SettingsProvider`, `SharedStorageBackup`, `Shell`, `SimAppDialog`, `SoundPicker`, `StatementService`, `Stk`, `StorageManager`, `SystemUI`, `Tag`, `TeleService`, `Telecom`, `TelecomServiceResources`, `TelecomShim`, `TelecomUi`, `TelephonyProvider`, `TelephonyProviderHsum`, `Traceur`, `UsbDisableDebugger`, `UserDictionaryProvider`, `VirtualDeviceManager`, `VpnDialogs`, `WallpaperBackup`, `aisealhostservice`, `messaging`, `webview`

## Java库/JNI（48）
`android.hidl.base-V1.0-java`, `android.hidl.manager-V1.0-java`, `android.test.base`, `android.test.mock`, `android.test.runner`, `androidx.window.extensions`, `androidx.window.sidecar`, `appfunctions.extension.xml`, `com.android.extensions.appfunctions`, `com.android.future.usb.accessory`, `com.android.hardware.biometrics.fingerprint.virtual`, `com.android.location.provider`, `com.android.media.remotedisplay`, `com.android.media.remotedisplay.xml`, `com.android.mediadrm.signer`, `com.android.nfc_extras`, `com.android.nfcservices`, `dex_bootjars`, `ext`, `framework-conscrypt-nsc`, `framework-minus-apex-install-dependencies`, `framework-network-security-config`, `framework-ondeviceintelligence-platform`, `framework-platformtelephony`, `framework-telecom`, `frameworks-base-overlays`, `frameworks-base-overlays-debug`, `gsi_overlay_framework`, `gsi_overlay_systemui`, `ims-common`, `javax.obex`, `libalarm_jni`, `libandroid_runtime`, `libandroid_servers`, `libandroidfw`, `libaudioeffect_jni`, `libdrmframework_jni`, `libmedia_jni`, `libmonkey_jni`, `librs_jni`, `librtp_jni`, `libvintf_jni`, `libwebviewchromium_loader`, `libwebviewchromium_plat_support`, `org.apache.http.legacy`, `services`, `telephony-common`, `voip-common`

## ART/zygote（15）
`app_process`, `bcc`, `cppreopts.sh`, `dirty-image-objects`, `hiddenapi-package-whitelist.xml`, `idmap2`, `idmap2d`, `init.zygote32.rc`, `init.zygote64.rc`, `init.zygote64_32.rc`, `ld.mc`, `odsign`, `otapreopt_script`, `preloaded-classes`, `zygote_next`

## Java框架配套服务(PLAN裁掉)（16）
`credstore`, `dnsmasq`, `gatekeeperd`, `incident`, `incident-helper-cmd`, `incident_helper`, `incidentd`, `installd`, `keystore2`, `mdnsd`, `monkey`, `ndc`, `netd`, `sdcard`, `uiautomator`, `vold`

## Java/服务apex（7）
`aosp_mainline_modules`, `com.android.apex.cts.shim.v1_prebuilt`, `com.android.cellbroadcast`, `com.android.crashrecovery`, `com.android.profiling`, `com.android.telephonycore`, `com.android.webapp`

## 显示栈（15）
`blank_screen`, `bootanimation`, `libEGL`, `libEGL_angle`, `libGLESv1_CM`, `libGLESv1_CM_angle`, `libGLESv2`, `libGLESv2_angle`, `libGLESv3`, `libgui`, `libinputflinger`, `screencap`, `screenrecord`, `sfdo`, `surfaceflinger`

`screencap`、`libinputflinger` 条目说明（2026-09-29 D 普查后更新）：
- `screencap`：deps 清单删除后仍经 `dumpstate` 的 required 传递拉入镜像
  （soong module-info 反查唯一引入点），已以平台补丁 0012 摘除 dumpstate/Android.bp 该条目。
- `libinputflinger`：同日曾按"输入/触摸栈非显示类"恢复保留（Android.bp 原注释），
  D 普查证实其宿主是 system_server InputManagerService 而非独立输入服务
  （镜像内仅 libandroid_servers/libservices.core 链接，二者均已删）——再下架，最终状态：删。

## PM/framework元数据XML（29）
`android.software.credentials.prebuilt.xml`, `android.software.preview_sdk.prebuilt.xml`, `android.software.webview.prebuilt.xml`, `android.software.window_magnification.prebuilt.xml`, `apns-full-conf.xml`, `app-lock-exempt.xml`, `approved-ogki-builds.xml`, `enhanced-confirmation.xml`, `framework-audio_effects.xml`, `framework-graphics`, `framework-location`, `framework-sysconfig.xml`, `initial-package-stopped-states.xml`, `kernel-lifetimes.xml`, `package-shareduid-allowlist.xml`, `platform.xml`, `preinstalled-packages-asl-files.xml`, `preinstalled-packages-base-product.xml`, `preinstalled-packages-gsi-system-ext.xml`, `preinstalled-packages-handheld-system-ext.xml`, `preinstalled-packages-media-product.xml`, `preinstalled-packages-media-system-ext.xml`, `preinstalled-packages-media-system.xml`, `preinstalled-packages-platform-generic-system.xml`, `preinstalled-packages-platform-handheld-system.xml`, `preinstalled-packages-platform-telephony-system.xml`, `preinstalled-packages-platform.xml`, `preinstalled-packages-strict-signature.xml`, `privapp-permissions-platform.xml`

## GSI版本探测rc垫片（2）——保留：官方模块+平台补丁0016
`init.gsi.rc`, `init.vndk-nodef.rc`
（2026-09-30 用户裁定：不删、也不在设备树搓副本，直接沿用官方模块
（gsi/Android.bp 定义，安装路径与官方一致：system_ext/etc/init/init.gsi.rc +
system_ext/etc/gsi/init.vndk-nodef.rc）。唯一改动是平台补丁 0016
（build/make 本地提交 8962f22）：nodef 的 `exec reboot bootloader` 补丁为
/dev/kmsg 告警继续——官方 nodef 是对过老 vendor 的保护性拒绝，picodroid 设计
运行 current-vendor（实测 ro.vndk.version 无探测逻辑、由 vendor build.prop
声明、CF 不写），官方行为必 bootloop。补丁版垫片曾以设备树副本实现，因与
stock 模块安装路径冲突（overriding commands）改为本方案。）

## AVF虚拟化(1)
`com.android.compos`
（AVF = Android Virtualization Framework 的 composition 服务；picodroid 无 VM 需求。
原误将 VNDK v31–v34 归入本类目，已纠正——那四项是 VNDK 快照且已恢复全包含。）

## Java框架UI音效数据（1）

`frameworks_sounds`（闹钟/铃声/通知/触控/UI 音效，prebuilt_media 组）——服务已删除的 SystemUI/UI 栈，无 UI 即死数据；且其 no_full_install 属性使 kati 侧结构性无法镜像（file_list_diff 双侧对齐无法达成）。属「Java 框架配套」类目延伸，特此单列备查。

## 结构性调整（非删除，逐项说明）

1. `soong_config_variables: TARGET_ADD_ROOT_EXTRA_VENDOR_SYMLINKS` 平铺为 default 分支取值
   （generic_symlinks + :plat_file_contexts）。该 soong_config 仅个别高通 BoardConfig 置位，
   picodroid BoardConfig 不置位，恒走 default——取值可证等价。
2. `aosp_mainline_modules`（元模块）删除后以 8 个保留 apex 显式列出：
   adbd、i18n、tzdata、resolv、media、media.swcodec、neuralnetworks、runtime。
   - com.android.runtime 为 bionic 支撑（libc/libm/linker/linkerconfig/crash_dump/libc++），
     非 ART（ART 在 com.android.art，已删）。RELEASE_DEPRECATE_RUNTIME_APEX 默认 false。
   - 决策记录（2026-09-29 用户拍板）：android17 下 apex 机制承重
     （adbd 仅存于 apex、mediaserver 链 i18n 的 libicu、TARGET_FLATTEN_APEX 已被上游移除），
     与 PLAN §3 "apexd 裁掉" 冲突，改为保留 apexd + 最小原生集。
3. `init.gsi.rc` / `init.vndk-nodef.rc`（VNDK 版本垫片）：保留官方模块；平台补丁 0016 把
   nodef 的 `reboot bootloader` 改为 kmsg 告警（picodroid 跑 current-vendor，ro.vndk.version
   不设，官方行为必 bootloop）。双轨：build/make 本地提交 8962f22 + patches/0016-*.patch。
4. VNDK v31–v34 apex：曾删；2026-09-30 用户裁定"必须全包含"恢复（对齐官方 GSI 通刷能力）。
5. make 侧不继承 generic_system.mk / gsi_system_ext.mk / gsi_product.mk / updatable_apex.mk /
   core_64_bit(_only).mk（Java 产品链与 zygote/apex-shim 混入），改以 picodroid_common.mk
   镜像 soong deps；PRODUCT_PACKAGES 与 Android.bp deps 锁步，由 file_list_diff 校验闭环。
6. 平台补丁 0001（system/core/rootdir）：init.rc 的 required 去除 platform-bootclasspath 与
   boringssl-zygote rc；init.rc 正文删 zygote import 两行与 conscrypt-apex boringssl 自测服务块。
   双轨：system/core 本地提交 + patches/0001-*.patch。
7. Wi-Fi/BT：无 NDK 面（public.libraries.android.txt 已核实）；com.android.wifi/bt apex 为
   system_server 侧 Java 栈，删除不影响 vendor HAL 存活（M2 验证）。
8. testsuite 二进制分发（2026-09-30 用户裁定）：pdtest_* 不进镜像产物，随 apps/ 契约
   bin/<uname -m>/ 携带（tools_sync_test_bins.sh 从构建产物同步、固定映射清单），
   adb push 到机器 /data/app/ 后由 launcher 拉起；start.sh 改 exec
   `$(dirname $0)/bin/$(uname -m)/pdtest_*`。镜像仅新增 picodroid-launcher
   （init rc 既契约 `service picodroid-launcher`，data.ready → exec_start，
   并代办 boot_completed/VIRTUAL_DEVICE_BOOT_COMPLETED 标记）。此前 H 轮 11/11
   的 pdtest_*/launcher 系 avbctl disable-verity + remount 运行时推入 /system/bin
   （非持久），镜像本体从无——本轮一并纠正。

## M1 期间追加变更（2026-09-29，上机迭代实证）

1. **lmkd 删除**（原"保留"）：其 libmemevents 静态初始化在本环境 abort 崩溃循环，
   且服务对象 ActivityManager(Java) 已裁——改判为 Java 框架配套。
2. **com.android.tethering apex 恢复保留**（原删）：NDK libandroid.so 链接其
   connectivity_native，ASensor/ACamera 功能依赖（保留 apex 集 8→9 个）。
3. **vdc 保留但平台补丁 0005**：fs_mgr 启动路径会调 vdc（无 vold 时每次阻塞 51s×N），
   补丁将等待循环 5000 次(50s)降为 100 次(1s)。
4. **平台补丁 0001 扩充**（system/core/rootdir/init.rc，全部属 vold/FBE/keystore/ART 类目）：
   - 移除 3 处阻塞型 `exec vdc`（checkpoint markBootAttempt/prepareCheckpoint、keymaster earlyBootEnded）
   - 移除 `installkey` / `init_user0`（失败即 reboot recovery）
   - 剥离全部 49 处 `mkdir ... encryption=` 策略（策略文件由 vold 写入；缺失即 reboot recovery）
   - 移除 post-fs-data 的 keystore/odsign/ART 等待（`wait_for_prop keystore.module_hash.sent`、
     `odsign.key.done`、derive_classpath/art_boot——无目标即永久阻塞）
   - 移除 zygote-start 触发器的 `wait_for_prop odsign.verification.done` + start statsd/zygote
5. **ro.crypto.type/state 抢先固化**（early-init 置 none/unsupported，ro. 先到先得）：
   阻止 fs_mgr 按 fstab fileencryption 标志置 file——init 的 mkdir 加密策略分支整体短路。
6. **/userdata 标签补丁 0004**（system/sepolicy）：e2fsdroid 需要镜像内每个目录有 plat 标签。
7. **sys.boot_completed=1 临时置位**（M1；M4 起 picodroid-launcher 接管）。
8. **testsuite 经验**：平台模块混链 c++_static 会与共享库符号插入冲突（libultrahdr/libmemevents
   静态初始化堆损坏 abort）——测试/应用二进制一律 c++_shared。

## 死件下架（2026-09-29，D 普查镜像内死件）

两侧锁步删除（device Android.bp deps + picodroid_common.mk PRODUCT_PACKAGES 同名同步）：

1. **`wificond`**（含 `wificond_compat_symlink_module` 符号链与 wificond.rc）——
   服务永不启动的死件；Android.bp 三处（system_ext select-default、gsi SHIPPING_API_LEVEL_33、
   common deps 的 compat 符号链）+ mk 两处，两侧同步删。
2. **`layertracegenerator`**——依赖已删的 SurfaceFlinger（debuggable select + PRODUCT_PACKAGES_DEBUG）。
3. **`update_verifier`**——宿主 OTA 流程（system_server 侧 update_engine 监听端）已删；
   update_verifier.rc 随模块一并出镜像。
4. **`libinputflinger`**——显示栈节条目说明更新（见上），随删。
5. **`libamidi`**——NDK AMidi 面向 Java app，镜像内无消费者（仅 CTS/JNI 测试链接）。
6. **`libgatekeeper`**——宿主 gatekeeperd 已删（Java框架配套服务节）；镜像内无其余链接者。
7. **`screencap`**——传递自 `dumpstate` required，经平台补丁 0012 摘除（frameworks/native
   cmds/dumpstate/Android.bp 去一行；make/soong 两侧经 required 机制对称生效，无 file_list_diff 偏差）。

传递依赖保留（无法直接删，镜像内仍在）：

- **`libbatterystats_aidl.so`**——传递自 `libsensorservice`/`libmediautils`（存活模块的
  shared_libs；BatteryNotifier 改造归 R2，未动）。
- **`libincident.so`**——传递自 `libperfetto_android_internal`（traced/perfetto 栈）。
- **`dropboxmanager_aidl-cpp.so`**（即 libdropboxmanager_aidl-cpp.so）——传递自
  `dmesgd`→`libservices` 与 `libperfetto_android_internal`→`libservices` 双路。
- `libattention_native.so`（+ `attentionmanager_native_aidl-cpp.so`）——传递自
  `libinputflinger`，随其下架自然出镜像，无需单删。

## 残余消噪（2026-09-29，system_server 服务缺失重复日志）

平台补丁 0012 + 各仓库本地提交（每处一行 `// picodroid: no system_server` 注释）：

1. **storaged**（system/core storaged_uid_monitor.cpp）：`get_uid_names` 删除 package_native
   查询——原每轮轮询 LOG(ERROR)+"getService package_native failed"；uid 名保持数字。
2. **libsensor**（frameworks/native Sensor.cpp / SensorManager.cpp）：
   - Sensor 构造：permission `getService`→`checkService`（原每传感器阻塞 5s 且
     libbinder 打 "Service permission didn't start" ALOGW）；
   - `getDeviceIdForUid`：virtualdevice_native 查询删除，恒返回默认设备；
   - `getInstanceForPackage`：permission getService→checkService，删
     "Cannot get permission service" ALOGE。
   （libsensor 为 sensorservice 与 app 进程共用，改动对两侧同为静默化。）
3. **nuplayer AWakeLock::acquire**（frameworks/av）：删除 power checkService 尝试——
   恒不持锁，重复 ALOGW 消失。
4. **audioflinger ThreadBase::getPowerManager_l**（frameworks/av）：置空，同上。
5. BatteryNotifier 相关由 R2 处理（frameworks/av 8d6fe11 等），本节跳过。

## 最小启动门控（2026-09-29，M4：开机零 class 启动）

平台补丁 **0013**（system/core init/builtins.cpp，双轨：本地提交 + patches/0013）：

1. `do_class_start` 开头读 `picodroid.boot.profile`，值为 `minimal` 时打一行
   `picodroid: minimal profile: skip class_start <class>` 并直接 `return {}`。
   行为变更：stock/vendor 的全部 `class_start`（core/main/hal/early_hal/late_start/charger）
   被短路，开机零 class 自动启动；显式 `start`/`exec_start`/`restart`/`ctl.start` 不受影响
   ——这是白名单服务与 launcher 按需拉起的唯一通道。
2. 开关：`rootdir/init.picodroid.userdata.rc` `on early-init`
   `setprop picodroid.boot.profile minimal`；删除该行即恢复 stock class 语义
   （`on boot` 的 nonencrypted 补触发器为兼容保留）。
3. 白名单链路逐项核实（android17 源码，均显式、非 class，无需为门控补动作）：
   ueventd（init.rc early-init 显式）、logd/servicemanager/hwservicemanager/
   vndservicemanager（on init 显式）、apexd（early-init exec_start + post-fs-data 显式
   restart；apexd.rc 三服务本就 disabled）、linkerconfig（builtin perform_apex_config 内
   GenerateLinkerConfiguration，是命令非服务）。adbd 例外：服务定义在 com.android.adbd
   apex rc（class core + disabled + override），apex rc 到 perform_apex_config 才解析、
   晚于 sys.usb.config 属性触发——在我们 rc `on property:apexd.status=activated` 兜底
   `start adbd`（幂等）。完整白名单与已知残留（tombstoned/aconfigd/console 等 stock
   显式 start 小驻留）见 README「最小启动架构」节与 rc 末尾验证清单注释。
4. 配套（设备树改动，非删除类）：sensorservice 声明加 `disabled`（launcher 按需拉起）；
   picodroid-launcher depends 新语法 `service::<init服务名>`（ctl.start 后轮询
   init.svc.<名>=running，默认超时 15s 可 PD_SVC_WAIT 覆盖，超时→app skipped 记
   launcher.log；不参与 app 拓扑环检测；setprop/getprop 抽为垫片函数，宿主单测
   /tmp/pdlsvc/run.sh 16 断言全过）；apps/*/depends 更新：pd_audio_1→audioserver、
   pd_camera_1→cameraserver（+pd_audio 共存）、pd_media_1/pd_encode_1→mediaserver、
   pd_sensor_1→sensorservice、pd_bluetooth_1 留空待设备侧 lshal 确认 vendor 服务名。

## M2-M5 合并轮（2026-09-30，subagent 流水线）

**平台补丁新增**：
- 0010 MediaCodec 编码路径 package_native 死等拆除（sIsHandheld=false）+ ProcessInfoService 快速失败 + UidObserver 注册删除
- 0011 SchedulingPolicyService 死循环拆除（本地 sched_setscheduler）+ NativePermissionController 默认放行 + OpPlayAudioMonitor 类删除 + AppOpsManager 默认 MODE_ALLOWED + BatteryNotifier no-op + libpermission whole-link 修复（AttributionSourceState vtable）
- 0012 libsensor/storaged/mediaplayerservice 消噪 + 死件下架（wificond/screencap/update_verifier/libinputflinger 等）
- 0013 init class_start 门控（picodroid.boot.profile=minimal：开机只白名单启动 adbd 链，class core/main/hal 全跳过）
- 0014 审查后续：PermissionChecker 默认放行、sensorservice BatteryService no-op、ServiceUtilities package_native 拆除、codeclist.generator 非阻塞回退等

**权限框架整体移除**：checkPermission/noteOp/appops 全部"默认有权限"（全 root 世界）。

**最小启动架构**：service::<init服务名> 依赖语法，launcher 按需 ctl.start + 等 running（15s/可调）。
**实测（CF）**：minimal 档开机进程仅 ~66（原 200+）；sensor/multihal→sensorservice 按需链路实证 PASS。

**app 形式测试结果（第一轮）**：pd_sensor=pd_bluetooth=pd_mount=PASS；pd_audio(-896 输入流)、pd_camera(0 相机)、pd_media(createCodecByName 挂起)进行中；hal/dns app 需按 depends 拉起 vendor HAL（已在树内修正）。
**服务名勘误**：mediaserver 的 init 服务名是 media。

**H 轮收敛根因（NDK 测试方法论，非平台缺陷）**：
- 裸 NDK 进程不自动起 binder 线程池：AMediaCodec dequeue/回调、ACamera 会话回调
  需要 `ABinderProcess_startThreadPool()`（pdtest_media/pdtest_encode/pdtest_camera 补齐后通）。
- idlcli 仅实装 vibrator 一类 idl（其余子命令 stub）——HAL 面测试不能依赖 idlcli，
  以 servicemanager 注册名 + 自写 binder 客户端为准（pdtest_hal 定稿形态）。
- 相机 vendor 限制：CF 虚拟相机不支持 ZSL/模板全枚举，STILL_CAPTURE 模板 +
  最小流配置才是可移植判据（测 Android 规范形态，不为平台 bug 改写测试）。
- AMediaCodec configure 必须带 width/height + csd；ACameraDevice_createCaptureSession
  回调结构体不可空；AMediaCodec_getInputBuffer 入队尺寸不得超过 cap 返回值。

## DSU 安全 userdata 挂载（2026-09-30，M3：行为变更，非删除）

**问题**：原 `picodroid-mount-userdata.sh` 契约为"mount ext4 by-name/userdata 失败 →
mke2fs 重建"。picodroid 作为 DSU guest（gsid 动态加载）运行时，guest 眼中的
/dev/block/by-name/userdata 是**宿主系统的 userdata 分区**——原逻辑会毁宿主数据。

**DSU 机制核实**（android17 源码：system/gsid、system/fs/fs_mgr、system/core/init）：
- gsid 激活 DSU 写 `/metadata/gsi/dsu/active`（libgsi.h kDsuActiveFile，gsi_service.cpp）；
  guest first_stage 另写 `booted`（MarkSystemAsGsi）与 `lp_names`（DSU 包镜像名列表）。
- guest 的 first_stage_mount（UseDsuIfPresent→MapAllImages，first_stage_mount_android.cpp）
  把 DSU 包内镜像经 device-mapper 暴露；userdata 虚拟设备 dm 名固定 **userdata_gsi**
  （libgsi.h kDsuUserdata），节点 /dev/block/dm-N，ueventd 建稳定链接
  **/dev/block/mapper/userdata_gsi**（init/devices.cpp）。
- 二阶段 init 在任何 rc action 前依据 booted 置 **ro.gsid.image_running**（init.cpp
  "Make the GSI status available before scripts start running"）——rc 侧 DSU 判定用。
- **官方语义**（fs_mgr/libfstab/fstab.cpp TransformFstabForDsu）：DSU 包含 userdata_gsi
  时 guest 的 /data 条目改指虚拟设备（formattable=1，官方允许 guest 格式化它——只会毁
  DSU 自带镜像文件，非宿主分区）；**不含时 /data 条目原样保留，guest 直接挂宿主真实
  userdata 分区、与宿主共享 /data**。

**行为变更**（rootdir/init.picodroid.userdata.rc + rootdir/picodroid-mount-userdata.sh）：
1. rc `on fs` 拆为两个属性分支先做 DSU 判定（exec_start 同步模式不变）：
   `on fs && property:ro.gsid.image_running=1` → exec_start picodroid-userdata-dsu
   （参数 dsu）；`=0` → exec_start picodroid-userdata（参数 normal）。设备路径决策
   全部留在脚本（脚本按参数 + 复核 active/booted 指示文件，指示文件在而参数说
   normal 时强制 DSU 路径——误判方向只可能更保守）。rc 不再 wait 任何设备（名字
   集与槽相关，只有脚本能解析）。
2. **mapper-only（2026-09-30 终版）**：设备路径一律 /dev/block/mapper/，by-name
   路径整体删除（用户真机实测非 DSU /data 即挂 mapper/userdata）。脚本唯一分支：
   DSU → userdata_gsi；非 DSU → userdata<slot> 优先、userdata 兜底（名字集取 10s
   内先出现者，等不到 FATAL exit 1 fail fast 不降级）。挂载失败（坏 fs）→ AUDIT +
   mke2fs 重建**该 mapper 设备** + e2fsck + 重挂——DSU 态重建对象是 gsid 虚拟设备
   （官方 TransformFstabForDsu formattable=1，只毁 DSU 包自带镜像文件）；非 DSU
   态重建自家逻辑设备（清数据契约保留）。宿主真实 userdata 分区（宿主 by-name
   设备）零引用，无写路径可达。
3. 审计日志：任一 mke2fs 前输出 "AUDIT: mke2fs device=... size=..."
   （blockdev --getsize64，取不到记 unknown）。
4. 日志统一 "picodroid-mount:" 前缀追加写 /dev/kmsg（写不进时回退 stdout）。

**设备名源码核实（android17 本树，文件:行号）**：
- dm 名 "userdata_gsi" 全链路同名、无前缀、无槽后缀：安装 createPartition
  ("userdata")（gsi_service.cpp:217-233）→ 镜像名=分区名+"_gsi"
  （partition_installer.cpp:297-299，kDsuPostfix libgsi.h:88）→ dm 名=镜像名原样
  （image_manager.cpp:298-320 MapWithDmLinear → fs_mgr_dm_linear.cpp:227-228
  device_name 空则取 partition_name、:244 dm.CreateDevice）→ fstab 改写同常量
  kDsuUserdata="userdata_gsi"（libgsi.h:91，fstab.cpp:559-561）且 slot_select=false
  （fstab.cpp:591）；guest first_stage 在改写 fstab 前已 MapAllImages 建好 dm
  （first_stage_mount_android.cpp UseDsuIfPresent）。
- 非 DSU 逻辑设备名由 fs_mgr_update_for_slotselect 决定（libfstab/slotselect.cpp:
  54-71）：slotselect 条目 blk_device 与 logical_partition_name 均追加
  ro.boot.slot_suffix → A/B-in-super 为 userdata_a/_b；super 非 A/B 无后缀
  （anland 真机实测 mapper/userdata）。
- dm 名→节点：ueventd 建 /dev/block/mapper/<dm名>（init/devices.cpp:539-543），
  本体 /dev/block/dm-N。

**实机验证（2026-09-30，CF adb :6520，slot=_a）**：
- mapper/ 下 apex 镜像以镜像名原样出现（com.android.adbd 等）——佐证 dm 名=镜像名
  规则，gsid 的 userdata_gsi 将同样原样出现。
- **CF 无 mapper/userdata 系名字**：CF 的 userdata 是物理分区（by-name/userdata →
  /dev/block/vda9），super 只含 system_a/vendor_a/product_a/system_ext_a/odm_a(+
  _dlkm_a) 与 *-verity 系。mapper 名字集在 CF 上无等价名——by-name 删除后 CF
  实例将 fail fast（data.ready 不置位）。CF 适配（userdata 入 super 或 bring-up
  期临时名字集扩展）另行立项；脚本内名字集构造是唯一改动点（NAMES= 一处）。

**主机单测**：/tmp/dsu-test/run.sh 由独立测试 agent 维护（与实现分离）；实现侧仅
sh -n 自查。

> **（2026-09-30 挂载重构后上节整体废止，见下节；保留作历史记录。）**

## 挂载重构（标准 /data + move mount + boot bind）（2026-09-30，用户多轮裁定定稿，行为变更非删除）

**方案一句话**：fstab 用单条目、标准形态挂 /data（DSU 换源走系统原生
TransformFstabForDsu）；挂载后 hook 用 move mount 把 /data 移到 /userdata，再建
/userdata/boot 子目录 bind 回 /data。取代上节 M3 mapper-only 方案（脚本挂载+
脚本 mke2fs 重建+picodroiddata bind），回归系统原生挂载路径。

**用户裁定记录**（多轮，定稿不再复议）：
- 标准 /data 形态挂载，不再脚本自挂；单条目 fstab，不加第二条备选。
- DSU 换源用系统原生 TransformFstabForDsu，脚本不再按态选设备。
- move mount（/data→/userdata）+ boot 子目录 bind 回 /data；boot 命名理由：
  userdata 后续装载 Linux rootfs，boot（容器启动环境驱动）作 picodroid 专数据区，
  避免容器应用错误清理。
- **合法 ext4 盘永不格式化**（优先级高于一切判据）：只要目标盘已是合法 ext4
  （偏移 1080 魔数 0xEF53）一律不触碰内容；**明确禁止依赖标记文件**做清零判断；
  boot 目录缺失只由 hook 的 mkdir 幂等重建（目录可重建、盘数据不可）。格式化只
  发生在"非 ext4 盘"且非 DSU 态，且只清零首 4096 字节交标准 formattable 链
  （脚本无任何 mke2fs）。
- 日志路径 /userdata/picodroiddata → /userdata/boot（launcher 与 testsuite 全量同步）。

**W 报告源码依据（android17 本树，本轮逐条复核通过）**：
- `libfstab/fstab.cpp:559-571` TransformFstabForDsu：DSU lp_names 含 userdata_gsi 时
  /data 条目 blk_device 改写为 userdata_gsi、强置 logical+formattable、slotselect 清零；
  `:676-690` ReadFstabFromFile 见 /metadata/gsi/dsu/booted 即调之——**对 mount_all
  显式路径同样生效**（builtins.cpp do_mount_all：非空 fstab_path 走 ReadFstabFromFile）。
- `fs_mgr.cpp:1192-1208` fs_mgr_update_logical_partition：`blk_device[0]=='/'` 直接
  return true（路径形态已解析、无需按名解析）→ 非 DSU 态（by-name 路径条目）logical
  为惰性旗标，`:1607-1612` 的 skip 分支不可达；DSU 态（裸名 userdata_gsi）按
  DeviceMapper 名解析出 /dev/block/mapper/userdata_gsi。**CF 判定：无冲突**——CF 的
  by-name/userdata（物理分区 vda9）直接挂，logical 不致失败，无需停。
- `fs_mgr.cpp:1707-1747` + `libcutils/partition_utils.cpp:42`：mount 失败 →
  partition_wiped（首 4096 全 0/全 ff）→ formattable → fs_mgr_do_format
  （mke2fs -b 4096 + e2fsdroid）；e2fsdroid 经 init_second_stage 的 required 进镜像
  （system/core/init/Android.bp:339），标准格式化链完整。
- `libfstab/slotselect.cpp:64-67`：slotselect 条目遇空 slot_suffix 整表失败——不加。
- `libfstab/fstab.cpp:222/633-650`：fsverity 旗标；tune 路径（fs_mgr.cpp:849 附近）
  无挂载点判据，显式补齐与真机标准一致。
- `system/gsid/partition_installer.cpp:301-325`：DSU 安装期 Format() 清零 userdata_gsi
  首 4K → DSU 态空白盘由标准 formattable 链自动重建，脚本零接触。
- `toys/lsb/mount.c:115`（`{"move", MS_MOVE}`）：toybox mount 支持 -o move；
  `system/core/init/builtins.cpp:465-483` mount 旗标表无 move → move mount 必须在
  shell 服务里做（hook 存在的理由）。
- `system/vold/MetadataCrypt.cpp:68`（kDmNameUserdata="userdata"）：真机实测
  /dev/block/mapper/userdata 的来源解释——vold dm-default-key 包装层，基底即
  by-name/userdata；picodroid 无 vold，直接挂基底，与标准形态一致。

**行为变更**（设备树内文件）：
1. 新增 `rootdir/fstab.picodroid`（prebuilt_etc → /system/etc/；名字不等于
   fstab.<suffix>，不被 GetFstabPath 当默认表）：单条目
   `/dev/block/by-name/userdata /data ext4 noatime,nodev,nosuid wait,check,quota,formattable,logical,fsverity`
   （零加密标志、无 slotselect、无 latemount/first_stage_mount）。
2. `rootdir/init.picodroid.userdata.rc` 挂载主体重写：
   `on early-fs`（provision → `mount_all /system/etc/fstab.picodroid` → hook）、
   `on late-fs`（hook2）；三服务 provision/hook/hook2（root/root/u:r:su:s0/disabled/
   oneshot）。early-init minimal profile+crypto 固化、sensorservice 声明、
   nonencrypted 补触发、boot_completed 段、白名单注释全部保留。旧 `on fs` 双分支
   exec_start 整段删除。
3. `rootdir/picodroid-userdata`（sh_binary，取代 picodroid-mount-userdata）三子命令：
   - provision：DSU 态（ro.gsid.image_running=1 或 booted 指示文件）exit 0 零接触；
     否则 mapper/userdata → by-name/userdata 兜底探测（仅探测），读偏移 1080 魔数
     0xEF53——合法 ext4 → exit 0 永不触碰；非 ext4 → AUDIT + 清零首 4096 + sync
     （交标准 wiped→formattable→fs_mgr_do_format）；找不到设备 FATAL exit 1。
   - hook：等 /proc/mounts 出现 /data（10s 超时 FATAL）；DSU 守卫（DSU 态 /data 源
     basename 必须 == userdata_gsi，否则 FATAL 防挂宿主分区）；`mkdir -p /userdata`；
     `mount -o move /data /userdata`；`mkdir /userdata/boot`（幂等重建，非格式化
     理由）；`mount -o bind /userdata/boot /data`。任一步失败 FATAL exit 1。
   - hook2：清宿主 mount_all --late 叠挂（循环 umount 至顶层恢复 /userdata/boot
     bind，umount 失败幂等忽略）；校验拓扑（/data 顶层源==/userdata/boot、
     /userdata 为 ext4）→ `setprop picodroid.data.ready 1`。
4. 删除 `rootdir/picodroid-mount-userdata.sh`；Android.bp / picodroid_common.mk
   deps 两侧同步换名（picodroid-mount-userdata → picodroid-userdata，新增
   fstab.picodroid）。
5. 日志/产物路径 /userdata/picodroiddata → /userdata/boot：picodroid-launcher.sh、
   testsuite/apps/*/start.sh、pdtest_all.sh、pdtest_camera.cpp 默认路径；
   pdtest_mount.cpp 判据整体改版（/data 源==/userdata/boot、/userdata ext4、
   DSU 守卫断言、bind 双向性、GSI 姿态旁证）。
6. 终态拓扑：/userdata=ext4 整分区（move 自标准 /data 挂载）；/data=/userdata/boot
   的 bind；/userdata/boot 之外区域预留给 Linux rootfs。

## SF 依赖砍除（2026-09-30，headless 全面扫除）

**背景**：SF 二进制/服务整体不存在；初版 0015 只把 AIDL `connectLocked` 的
waitForService 改 checkService（且 checkService 模板在该命名空间不存在，编译失败）。
本轮升级为彻底版：**零查找、零服务名、零阻塞**。

**libgui（frameworks/native，commit 4ade0e2，覆盖 666fd67）**：
1. `ComposerService`/`ComposerServiceAIDL` 的 `connectLocked` 均改为
   `mComposerService = nullptr; return false;`——legacy 路径（服务名 "SurfaceFlinger"）
   的 waitForService 原本会让 **Transaction::apply 永久挂起**，与 AIDL 路径同根同源。
2. SurfaceComposerClient 全部 ~50 个持 composer 的转发函数逐一加 null 守卫，按
   "无显示"语义降级：显示 token/信息类 → NAME_NOT_FOUND；全局能力类 → NO_INIT；
   bool/句柄/optional → false/nullptr/nullopt；纯注册/通知类 void → 直接返回；
   移除类（removeXxxListener/unregisterShader）→ OK（远端本就未注册）。
   `Transaction::apply`：丢弃状态后返回 **OK**——返回错误会触发 BLASTBufferQueue 的
   LOG_ALWAYS_FATAL，而无 composer 时 SurfaceControl 本就不可能创建成功（
   createSurfaceChecked 因 mStatus==NO_INIT 拒绝），该路径实际不可达，OK 语义为
   "事务投向虚空"；注意同步事务回调（TransactionCommittedCallback）不会触发，
   同步 apply 的 syncCallback 不等待（不阻塞）。
3. `DisplayEventReceiver`：未连接接收器是常态，initCheck 去 LOG_ALWAYS_FATAL 改返
   NO_INIT（cameraserver Camera3OutputStream 的 syncTimestampToDisplayLocked 对
   getLatestVsyncEventData!=OK 已有优雅降级——沿用）；构造函数 sf==null 分支本就跳过。
4. `BLASTBufferQueue::initialize`：删除 getMaxAcquiredBufferCount 查询，保持默认
   max-acquired=1；本地 buffer queue 照常工作（不 connect）。
5. `Surface`：getDisplayRefreshCycleDuration → NAME_NOT_FOUND；
   querySupportedTimestampsLocked → 直接返回（不支持 DISPLAY_PRESENT 时间戳）。
6. `WindowInfosListenerReporter`：add/remove 对 null composer 只做本地登记/清理。
7. ScreenshotClient 三处本就有 null 检查（NO_INIT），未动。

**frameworks/av（commit d49d36c）**：
- `VideoFrameScheduler::updateVsync`：删除 SurfaceFlingerAIDL checkService 探测
  （原每 30s 刷新周期打一次 servicemanager），mVsyncPeriod 恒 0 走壁钟默认。
- `Camera3BufferManager`：删除从未使用的 ComposerService/ISurfaceComposer include。
- 相机显示旋转/rotate-and-crop 全链核查：libcameraservice 仅消费 compat-info 携带的
  ui::Rotation 值（Camera2ClientBase/CameraProviderManager），无直接 composer 调用点；
  Java 侧 CameraServiceProxyWrapper.getRotateAndCropOverride 此前已由 0009 处理，无漏。

**无需改动（核实）**：
- libui/Gralloc4/5 的 IAllocator 是独立 gralloc HAL（android.hardware.graphics.allocator），
  与 SF 无关，保持 waitForService（HAL 存在）。
- nativewindow/bufferqueue 无 composer 引用（bufferqueue 并入 gui/，无独立目录）。
- frameworks/base/native/android（libandroid）：surface_control.cpp 仅用
  ISurfaceComposerClient 常量 + 经 libgui 受控路径（initCheck!=NO_ERROR → 返回
  nullptr），无需改动。
- gui/ConsumerBase、GLConsumer、BufferQueueCore 残留的死 ComposerService.h include
  （上游即死代码）无运行时影响，未动（避免无谓编译风险）。
- av 内其余 waitForService 均为 media/camera 自有服务（media.player、
  media.resource_manager、camera provider HAL），非 SF 依赖，不在本轮范围。

**AIDL ABI 锚点返工（首建后）**：删掉 waitForService<gui::ISurfaceComposer>()
模板的同时删掉了 libgui_aidl_static 归档成员（Bn/Bp ISurfaceComposer、
IRegionSamplingListener、IFpsListener、各 parcelable 共 ~405 符号）被拉入
libgui.so 的唯一引用 → libandroid_runtime（BnRegionSamplingListener 子类）链接
失败。修复：`picodroid_keepSurfaceComposerAidlAbi()`（SurfaceComposerClient.cpp）
以 `asInterface(nullptr)` 恢复同一锚点——纯本地空转换（宏实现里 obj==nullptr
直接返回空 sp），零 servicemanager 查找、零 binder 流量；拉回的是生成器的
真实现而非手写空壳，ABI 与上游一致。

**补丁**：0015-libgui-composer-failfast.patch 重写（853 行，7 文件，native 5 + av 2；
基线 frameworks/native=ae266dc、frameworks/av=475269e；双轨各仓本地提交
4ade0e2 / d49d36c）。

## CPU 优先级策略（2026-09-30 用户裁定：不动）

top-app cgroup 层级由 init 从 task_profiles.json 正常创建（/dev/cpuctl/top-app 等齐全）；
各服务按 task_profile 静态落组（audio/media→foreground，camera HAL→top-app）。
**不引入**把 picodroid app 迁入 top-app 的机制（stock 中该迁移由 system_server 随前台
切换执行，picodroid 无此场景）；app 进程落根组。如后续容器驱动需要差异化优先级再议
（候选：launcher 写 cgroup.procs 或 depends 声明式 prio:: 语法）。
