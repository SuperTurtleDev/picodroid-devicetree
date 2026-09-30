# picodroid testsuite

NDK **功能级**测试（非存活检查）：相机真拍 JPEG、音频真录/真放、传感器真收事件、
媒体真编解码闭环、蓝牙 AIDL HAL 真连接（NDK 无蓝牙 API 面，见 TRIM-LEDGER）。

## 用例
| 二进制 | 内容 | 判据 |
|---|---|---|
| pdtest_camera | ACameraManager→open→STILL_CAPTURE 拍照 | JPEG 文件 ≥8KB 且 FFD8 头 |
| pdtest_audio | AAudio 录 2s + 放 1s + OpenSLES 录音器 | 录音帧数≥1s 采样数、播放帧数达标 |
| pdtest_sensor | 枚举 + 事件队列 | 5s 内加速计事件 ≥3 |
| pdtest_media | c2.android.avc 编码 30 帧→解码 | 码流≥1KB 含关键帧、解码帧数≥25 |
| pdtest_bluetooth | IBluetoothHci binder 实例获取 | 服务获取成功且 alive |
| pdtest_mount | 挂载契约判据（标准 /data + move mount + boot bind） | /data 源==/userdata/boot、/userdata ext4、DSU 守卫、双向可见 |
| pdtest_all | 串行汇总 | 聚合 PASS/FAIL |

## 运行
- 测试二进制**不进镜像**（用户裁定 2026-09-30）：随 app 契约 `bin/<uname -m>/` 携带。
  构建后同步进契约：`tools_sync_test_bins.sh <product-out-arch-dir> <x86_64|arm64>`
  （固定映射清单即契约；arm64 落 `bin/aarch64/`，uname -m 口径）。
- 上机：`adb push apps/* /data/app/`（契约含二进制，push 即自足），由 launcher 拓扑拉起，
  日志按契约落 `/userdata/boot/logs/<id>.log`；手动补跑：`adb shell /system/bin/picodroid-launcher`。
- M1 调试期直跑：`adb shell /data/app/pd_media_1/bin/$(uname -m)/pdtest_media`
  （产物落 `$PD_OUT`，默认契约路径，回退 /data/local/tmp）。

## 边界说明
- 蓝牙 NDK 无 API（public.libraries.android.txt 核实）；v1 为 HAL binder 功能连接，
  HCI RESET→commandComplete 往返在 M2 平台补丁后启用（rootcanal 虚控器应答）。
- 相机/音频 uid 限制的放开属 M5 平台补丁；当前以 root 运行（PLAN §1 既定路线）。
