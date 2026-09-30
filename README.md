# anland-picodroid 设备树

无头 HAL-only Android GSI（定位与决策见 /work/anland-picodroid/PLAN.md）。

**M1 已达成（2026-09-29）**：`anland_picodroid_x86_64` 在 Cuttlefish 进 adb shell，
userdata 契约与 GSI 分区姿态（/system_ext、/product 符号链 + skip_mount.cfg）均上机实证。

**挂载重构终版（2026-09-30，用户多轮裁定）**：fstab 单条目标准形态挂 /data
（`rootdir/fstab.picodroid` → /system/etc/；DSU 换源走系统原生 TransformFstabForDsu）；
挂载后 hook 用 `mount -o move` 把 /data 移到 /userdata，再建 /userdata/boot 子目录
bind 回 /data；hook2 清宿主叠挂后置 picodroid.data.ready=1。核心原则：**合法 ext4 盘
永不格式化**（provision 只对非 ext4 盘清零首 4K，交标准 wiped→formattable→
fs_mgr_do_format；禁止依赖标记文件；boot 目录缺失只重建目录）。

## 结构
- `Android.bp` —— picodroid system 镜像：复制 android17 GSI 四个镜像 defaults 后按台账删减，
  分区姿态与 GSI 一致（单一自足 system + /system_ext、/product 顶层符号链 + skip_mount.cfg）。
  根目录含 /userdata 挂载点。
- `picodroid_common.mk` —— 公共产品配置（复制 gsi_release.mk 删减 + soong deps 的 make 侧镜像）。
- `picodroid_x86_64.mk` / `picodroid_arm64.mk` —— 双产品（CF 迭代 / 真机 GSI），同步维护。
- `x86_64/` `arm64/` —— BoardConfig（复制 device/generic/* 后删 32 位次架构）。
- `rootdir/init.picodroid.userdata.rc` —— userdata 挂载契约序列（/system/etc/init/ 自动解析）。
- `rootdir/fstab.picodroid` —— 单条目标准 /data fstab（/system/etc/fstab.picodroid；只由
  我们 rc 的 mount_all 显式消费，名字不等于 fstab.<suffix> 不当默认表）。
- `rootdir/picodroid-userdata` —— 契约脚本（provision/hook/hook2 三子命令，exec_start
  同步服务，su 域）。
- `rootdir/init.picodroid.rc` —— init.rc 裁剪参考副本（生效途径为 patches/0001 平台补丁）。
- `testsuite/` —— NDK 功能级测试（相机拍照/音频录放/传感器采样/媒体解码/蓝牙 HAL/触摸 evdev/
  挂载契约）+ picodroid app 契约包装（apps/）。adb push 部署，`pdtest_*` 直跑。
- `patches/` —— 平台补丁双轨（git 本地提交 + .patch + apply.sh）：
  - 0001 rootdir init.rc（zygote/boringssl/vdc/FBE/keystore/odsign 等）
  - 0002 soong build.prop visibility
  - 0003 filelistdiff allowlist（镜像自暂存隐式文件）
  - 0004 sepolicy /userdata 标签
  - 0005 vdc 无 vold 快速失败
- `TRIM-LEDGER.md` —— 裁剪台账：每一处删除/恢复的类目与理由；凡不在表者一律保留。
- `tools_gen_device_tree.py` —— 设备树生成器（复制 GSI defaults + 台账过滤）。

## 构建
```bash
cd /work/aosp && source build/envsetup.sh
lunch anland_picodroid_x86_64-trunk_staging-userdebug   # 注意 android17 需带 release 段
m systemimage
```

## 验证（Cuttlefish）
```bash
# 1. 重组 super + vbmeta_system（system 哈希变则 vbmeta 需重建；链式 vbmeta 不允许 flags 伪造）
cd /work/picodroid-cvd
PATH=/work/cvd/bin:$PATH build_super_image dict.txt super.img
avbtool make_vbmeta_image --include_descriptors_from_image <新system.img> \
  --include_descriptors_from_image extracted/product_a.img \
  --include_descriptors_from_image extracted/system_ext_a.img \
  --key /work/aosp/external/avb/test/data/testkey_rsa4096.pem \
  --algorithm SHA256_RSA4096 --rollback_index 1780617600 --padding_size 4096 \
  --output vbmeta_system2.img && truncate -s 65536 vbmeta_system2.img
# 2. 启动（注意：pkill 用 -x 精确名，-f 会自杀；无 timeout 杀前台 create）
cvd create --host_path=/work/cvd --product_path=/work/cvd \
  --super_image=.../super.img --vbmeta_system_image=.../vbmeta_system2.img --daemon
# 3. 测试（M1 调试期 adb push 直跑）
adb root && adb push <intermediates>/pdtest_* /data/local/tmp/
adb shell 'PD_OUT=/data/local/tmp /data/local/tmp/pdtest_<name>'
```

## 测试现状（M1 末）
| 用例 | 结果 | 说明 |
|---|---|---|
| pdtest_mount | 判据已改版 | 新判据：/data 源==/userdata/boot、/userdata ext4、DSU 守卫、GSI 姿态（挂载重构后待复测） |
| pdtest_bluetooth | PASS | IBluetoothHci binder 实连（rootcanal）；HCI 往返 M2 |
| pdtest_touch | PASS | 多点数字化仪能力探测 + 输入 HAL 实连 |
| pdtest_camera | 功能受限 | 链接/初始化已通；cameraserver→system_server 依赖待 M5 补丁 |
| pdtest_audio | 功能受限 | AAudio -896 断连；audioserver→activity 依赖待 M5 补丁 |
| pdtest_sensor | 功能受限 | sensorservice 等待挂起；同上 |
| pdtest_media | 功能受限 | 解码闭环已通编译；codec 服务路径待 M2/M5 |

（M2 服务三件套存活 + M5 平台补丁逐项过关后，上表受限项应转 PASS。）

## 最小启动架构（M4：开机零 class，服务按需拉起）

目标：开机默认不启动任何 class（core/main/hal/early_hal/late_start/charger 全不自动起），
只保留 adbd 及其硬依赖（init/ueventd/servicemanager/logd/apexd/linkerconfig），极致省内存。

### 原理

- 平台补丁 **0013**（system/core init/builtins.cpp，双轨：本地提交 + patches/0013）：
  `do_class_start` 开头检查属性 `picodroid.boot.profile`，值为 `minimal` 时打一行
  `picodroid: minimal profile: skip class_start <class>` 日志并直接返回——stock 与 vendor
  rc 里所有 `class_start`（含 vendor hal/early_hal、init.rc 的 hal/core、nonencrypted 链的
  main/late_start、charger）全部短路。**显式 `start` / `exec_start` / `restart` / `ctl.start`
  不受影响**，这是白名单服务的进入通道。
- 开关由我们 rc 固化：`rootdir/init.picodroid.userdata.rc` 的 `on early-init` 里
  `setprop picodroid.boot.profile minimal`（关闭最小启动 = 删这行，stock class 语义即恢复，
  nonencrypted 补触发器为兼容保留）。

**DSU 语义（挂载重构终版，取代 M3 mapper-only）**：fstab 单条目标准形态
（/dev/block/by-name/userdata → /data，formattable+logical），DSU 态换源走系统原生
TransformFstabForDsu（对 mount_all 显式路径同样生效，fstab.cpp:676-690）——DSU 包含
userdata_gsi 时 /data 条目自动改指 gsid 虚拟设备（formattable=1 官方语义，重建只毁
DSU 包自带镜像文件）；hook 里 DSU 守卫断言 /data 源 basename==userdata_gsi，DSU 包
不含 userdata_gsi 时（官方语义会共享宿主 /data）fail fast 拒收，绝不写宿主分区。
非 DSU 态：by-name 路径形态设备直接挂（fs_mgr_update_logical_partition 对 '/' 开头
设备短路返回，logical 为惰性旗标）——CF（物理分区）与真机同形，无需分支。
空白盘由标准 wiped→formattable→fs_mgr_do_format（mke2fs+e2fsdroid）重建；合法
ext4 盘永不格式化（见上）。源码行号与行为变更见 TRIM-LEDGER「挂载重构」节。

### 开机白名单（逐项核实过链路，均不依赖 class_start）

| 服务 | 启动链路（android17 实测源码） |
|---|---|
| ueventd | init.rc `on early-init` 显式 `start ueventd`（virtio 等 /dev 节点必需） |
| logd | init.rc `on init` 显式 `start logd`（先于 servicemanager；adb logcat 依赖） |
| servicemanager | init.rc `on init` 显式 `start servicemanager`（同批 hwservicemanager/vndservicemanager 亦显式，binder/HIDL 注册需要，一并保留） |
| apexd | early-init `exec_start apexd-bootstrap` + post-fs-data 显式 `restart apexd`；apexd.rc 三服务本就 disabled，从不走 class core |
| linkerconfig | /linkerconfig 由 builtin `perform_apex_config`（early-init 与 post-fs-data 显式调用）内的 GenerateLinkerConfiguration 生成——是命令不是服务 |
| adbd | 服务定义在 com.android.adbd apex 的 adbd.rc（class core + disabled + override）；apex rc 到 perform_apex_config 才解析，stock 靠 sys.usb.config 属性触发 `start adbd`（显式、不受门控），触发早于解析时会落空——我们 rc 里 `on property:apexd.status=activated` 兜底 `start adbd`（幂等） |
| sensorservice | 我们自己声明的服务，已加 `disabled`，不自动起 |

已知残留（stock init.rc **显式** start，非 class，门控不减，均为小驻留）：tombstoned、
system_aconfigd_socket_service / mainline_aconfigd_*（flag 存储）、console（仅 debuggable）。
lmkd/vold 的显式 start 因二进制已裁只剩 no-op 报错。minimal 模式验证：开机稳定后 `ps -A`
除内核线程外应只见 init/ueventd/logd/servicemanager/hwservicemanager/vndservicemanager/adbd
（apexd oneshot 已退出）——清单已写入 rc 注释。

### app 的 service:: 依赖（launcher 按需拉起）

`apps/*/depends` 每行一个依赖，两种语法：

```
<appid>                  # app 依赖：拓扑序 + 环检测（原有语义）
service::<init服务名>     # 运行前置的 init 服务
```

launcher（rootdir/picodroid-launcher.sh）在判定某 app 可执行前，对其所有 `service::` 依赖：
`setprop ctl.start <名>`（幂等）→ 轮询 `getprop init.svc.<名>`，`running` 即通过；`restarting`
视为继续等待；超时（默认 15s，`PD_SVC_WAIT` 可覆盖）仍不 running → 该 app skipped 并记
launcher.log（下游依赖它的 app 连带 skipped）。`service::` 行不参与 app 拓扑环检测。
当前映射：pd_audio_1→audioserver、pd_camera_1→cameraserver（+pd_audio app 依赖共存）、
pd_media_1/pd_encode_1→mediaserver、pd_sensor_1→sensorservice；pd_bluetooth_1 的 vendor
蓝牙 HAL 服务名待设备侧 `lshal` 确认后填入。

宿主单测：`/tmp/pdlsvc/run.sh`（setprop/getprop 抽为垫片函数——真机走 toolbox，宿主以
$T/prop 文件模拟；ctl.start 由 PD_HOST_SVC 钩子脚本模拟 init）。用例：① 拉起+等待成功后
app 运行（且已 running 不重复 ctl.start）② 超时不 running→skipped 连带下游 ③ 无 service::
行为不变（含环检测回归）——16 项断言全过（dash/bash 双 shell）。

### 如何加新服务到白名单

1. 查该服务的 rc：若是 `class <X>` 自动起（被 0013 门控），在我们 rc 里加显式动作，如
   `on property:apexd.status=activated`（或合适的属性/触发点）下 `start <服务名>`；
   只加 `start`，不动 stock rc。
2. 若仅个别 app 需要，**不要进白名单**——在其 `depends` 写 `service::<服务名>`，由
   launcher 按需拉起（vendor HAL 服务名可用 `lshal`/`service list` 上机确认）。
3. 验证：`getprop picodroid.boot.profile` = minimal；`getprop init.svc.<名>` = running；
   dmesg 里应有 0013 的 `skip class_start` 行（每个被短路的 class 一行）。
