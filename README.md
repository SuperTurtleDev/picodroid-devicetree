# picodroid 设备树

无头 HAL-only Android GSI —— 裁掉 Java/ART/显示栈，保留 binder 与全部纯 native 服务、
NDK 面（音频/相机/传感器/媒体/蓝牙 HCI/触摸/DRM/NFC）以及 HAL 运行环境，作为 Linux
容器的驱动垫片（GUI 与应用逻辑由容器内 Linux 用户空间接管）。

- 计划与决策：`/work/anland-picodroid/PLAN.md`（裁剪台账见 TRIM-LEDGER.md）
- 验证基线：Cuttlefish x86_64（Android 17 / android17-release）
- 状态：**11/11 launcher app 全部 PASS**（2026-09-30，见下表）

## 一、架构总览

```
┌─ 最小启动（picodroid.boot.profile=minimal）─────────────────────────┐
│ init / ueventd / servicemanager / logd / apexd(oneshot) / adbd     │
│ class core/main/hal 全部被门控跳过（补丁 0013），开机 ≈66 进程      │
│ 温控+system_suspend 白名单必拉（camera/audioserver 的阻塞依赖）     │
└──────────────────────────────────────────────────────────────────────┘
        │ picodroid.data.ready=1
        ▼
┌─ 挂载契约（PLAN §4，标准机制优先）───────────────────────────────────┐
│ early-fs: provision(非ext4盘→清零首4K，合法ext4永不格式化)          │
│           → mount_all /system/etc/fstab.picodroid（标准 fs_mgr：   │
│             DSU 态由 TransformFstabForDsu 自动换源 userdata_gsi；  │
│             wiped→formattable 自动 mke2fs+e2fsdroid）              │
│         → hook: bind /data /userdata; umount /data;               │
│                mkdir /userdata/boot; bind /userdata/boot /data     │
│ late-fs:  hook2（清宿主 --late 叠挂 → 重建 bind → data.ready=1）    │
└──────────────────────────────────────────────────────────────────────┘
        ▼
┌─ picodroid-launcher（PLAN §7 应用契约）────────────────────────────┐
│ /data/app/<id>_<ver>/  start.sh + depends                         │
│ depends: 每行一个依赖 id（Kahn 拓扑，失败/环→skipped 连带传播）     │
│          service::<init服务名> → ctl.start + 等 running(PD_SVC_WAIT)│
│ 日志 /userdata/boot/logs/<id>.log，状态 picodroid.apps.<id>=        │
│ {ok|fail|skipped}；完成后置 sys.boot_completed 与 cvd 启动标记      │
└──────────────────────────────────────────────────────────────────────┘
```

## 二、目录

| 路径 | 内容 |
|---|---|
| `Android.bp` | picodroid system 镜像（复制 android17 GSI 四个镜像 defaults 后按台账删减；分区姿态与 GSI 一致：/system_ext、/product 顶层符号链 + skip_mount.cfg；含 /userdata 挂载点目录） |
| `picodroid_common.mk` | 公共产品配置（gsi_release 复制裁减 + soong deps 的 make 侧镜像，file_list_diff 校验闭环） |
| `picodroid_x86_64.mk` / `picodroid_arm64.mk` | 双产品（CF 迭代 / 真机 GSI），同步维护 |
| `x86_64/` `arm64/` | BoardConfig（复制 device/generic/* 后删 32 位次架构，VNDK=current） |
| `rootdir/fstab.picodroid` | 单条目标准挂载（见上；零加密标志） |
| `rootdir/picodroid-userdata.sh` | provision / hook / hook2 三子命令 |
| `rootdir/init.picodroid.userdata.rc` | 挂载序列 + 最小启动白名单 + launcher + sensorservice 按需声明 |
| `rootdir/picodroid-launcher.sh` | 应用契约启动器（拓扑 + service:: 拉起） |
| `testsuite/` | NDK 功能测试（下表）+ `apps/`（12 个 picodroid app 契约包装） |
| `patches/` | 平台补丁 0001–0015 双轨（git 提交 + .patch + apply.sh） |
| `TRIM-LEDGER.md` | 裁剪/恢复全量台账：每处删除的类目与理由；M1–M5 迭代记录；各阶段用户裁定 |
| `tools_gen_device_tree.py` | 设备树生成器（复制 GSI defaults + 台账过滤） |

## 三、测试（全部通过）

| App | 判据 |
|---|---|
| pd_sensor | 传感器枚举 + 事件队列收包 |
| hal | health/powerstats/thermal/vibrator/memtrack/keymint/suspend/nfc 八项注册与事务证据 |
| pd_media | AMediaCodec AVC 解码（内置 BBB 码流，60 帧） |
| pd_encode | AMediaCodec AVC 编码（5973B 码流 + csd） |
| pd_audio | AAudio 录 2s + 放 1s + OpenSLES 双向 |
| pd_audio_ll | LOW_LATENCY 小 buffer 流 + 数据回调真实执行 |
| pd_audio_appuid | uid 10086 下录放（权限恒真实证） |
| pd_camera | NDK 拍照：640x480 JPEG ≥8KB 且 FFD8 头 |
| pd_bluetooth | IBluetoothHci binder + HCI RESET 往返 |
| pd_touch | 多点数字化仪能力探测 + 输入 HAL 实连 |
| pd_mount | 挂载契约（/data=bind /userdata/boot、/userdata ext4） |
| dns | IP 直连（DNS 降级属无 netd 预期） |

测试纪律：**符合 Android NDK 规范的程序必须原样可跑**——测试只允许修自身对 NDK
API 的误用，不为迎合平台缺陷而改写（用户裁定）。

## 四、构建与验证

```bash
cd /work/aosp && source build/envsetup.sh
lunch anland_picodroid_x86_64-trunk_staging-userdebug   # android17 需带 release 段
m systemimage
```

Cuttlefish 部署（system 哈希变则 vbmeta 需同链重建；宿主看门狗认内核日志标记）：

```bash
cd /work/picodroid-cvd
PATH=/work/cvd/bin:$PATH build_super_image dict.txt super.img
avbtool make_vbmeta_image \
  --include_descriptors_from_image extracted/product_a.img \
  --include_descriptors_from_image extracted/system_ext_a.img \
  --key /work/aosp/external/avb/test/data/testkey_rsa4096.pem \
  --algorithm SHA256_RSA4096 --rollback_index 1780617600 --padding_size 4096 \
  --output vbmeta_system2.img && truncate -s 65536 vbmeta_system2.img
cvd create --host_path=/work/cvd --product_path=/work/cvd \
  --super_image=.../super.img --vbmeta_system_image=.../vbmeta_system2.img --daemon
cd testsuite && tools_sync_test_bins.sh <out-arch-dir> x86_64  # 测试二进制装入契约
adb root && adb push apps/* /data/app/    # 契约（含 bin/<arch>/ 二进制，不进镜像）
adb shell chmod -R 755 /data/app/*/bin /data/app/*/start.sh   # adb push 不带执行位
adb shell /system/bin/picodroid-launcher  # 拓扑拉起；日志 /userdata/boot/logs/<id>.log
```

运维坑（历次实证）：pkill 用 `-x` 精确进程名（`-f` 会杀掉自身）；被 timeout 杀过的
adb 会话用 `disconnect + kill-server + connect` 复位；构建前 `unset -f grep`。

## 五、裁剪与依赖治理摘要（详见 TRIM-LEDGER.md）

- **删除**：Java/ART 全家、显示栈（SF/libgui 调用点/Composer 死等全部拆除，补丁 0015）、
  vold/keystore/incidentd/netd/lmkd/statsd(恢复为依赖)、system_server 服务的一切
  getService/waitForService 调用（补丁 0008–0014，权限语义=默认有权限）
- **保留 9 个 apex**（承重实证）：adbd/i18n/runtime/tzdata/resolv/media/swcodec/
  neuralnetworks/tethering/configinfrastructure/statsd
- **CPU 优先级**：服务按 task_profile 静态落组；app 不做 top-app 迁移（裁定：不动）
- **已知限制**：预编译 vendor 相机 provider 的 ZSL 路径零产出帧（用 PREVIEW+JPEG 流）；
  无 netd 故 DNS 降级（IP 直连正常）；`service list`/`dumpsys <HAL>` 挂起（测试绕行）

## 六、仓库

- 设备树：https://github.com/SuperTurtleDev/picodroid-devicetree
- 平台补丁：`patches/`（对 AOSP 各仓库的 git 提交在构建机本地，patch 文件随树走）
