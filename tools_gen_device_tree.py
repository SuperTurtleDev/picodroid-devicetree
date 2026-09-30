#!/usr/bin/env python3
"""picodroid 设备树生成器（初始 bootstrap 用；已完成使命）。

!! 警告：设备树自 bootstrap 后由人工维护，TRIM-LEDGER.md 为权威台账；
!! 重新运行本脚本会用初始版本覆盖人工修改（如 VNDK v31-34 恢复、GSI rc
!! 垫片保留官方模块+补丁0016 等）。仅供追溯初始生成逻辑，勿直接重跑。

纪律（LESSONS.md + 本轮纠偏）：
1. 四个 GSI 镜像 defaults 原文复制，只删台账条目（三类：Java/ART及配套、显示栈、VNDK垫片）
2. 保留 multilib/arch/select 结构与全部姿态属性
3. make 侧 PRODUCT_PACKAGES 与 soong deps 镜像（file_list_diff 校验）
4. init.rc 走平台补丁轨（PLAN §6 双轨）
"""
import re, os, json, subprocess

GENERIC_BP = '/work/aosp/build/make/target/product/generic/Android.bp'
GSI_BP = '/work/aosp/build/make/target/product/gsi/Android.bp'
ROOTDIR = '/work/aosp/system/core/rootdir'
OUT = '/work/aosp/device/anland/picodroid'

# ================= 台账 =================
R = {}  # name -> category
def _cat(cat, names):
    for n in names: R[n] = cat

_cat('Java应用', [
    'BackupRestoreConfirmation','BasicDreams','BlockedNumberProvider','BluetoothMidiService',
    'BookmarkProvider','BuiltInPrintService','CalendarProvider','CallLogBackup',
    'CameraExtensionsProxy','CaptivePortalLogin','CarrierDefaultApp','CellBroadcastLegacyApp',
    'CertInstaller','CompanionDeviceManager','ContactsProvider','CredentialManager',
    'DeviceAsWebcam','DeviceDiagnostics','DocumentsUI','DownloadProvider','DownloadProviderUi',
    'DynamicSystemInstallationService','E2eeContactKeysProvider','EasterEgg','ExtShared',
    'ExternalStorageProvider','FusedLocation','HTMLViewer','InputDevices','IntentResolver',
    'KeyChain','LiveWallpapersPicker','LocalTransport','ManagedProvisioning','MmsService',
    'MtpService','MusicFX','NetworkStack','ONS','PacProcessor','PackageInstaller',
    'PartnerBookmarksProvider','PrintRecommendationService','PrintSpooler','PrivateSpace',
    'ProxyHandler','SettingsProvider','SharedStorageBackup','Shell','SimAppDialog','SoundPicker',
    'Stk','Tag','TeleService','Traceur','UserDictionaryProvider','VpnDialogs','WallpaperBackup',
    'Launcher3QuickStep','Provision','Settings','StorageManager','SystemUI','CarrierConfig',
    'Browser2','Camera2','Dialer','LatinIME','messaging','SatelliteClient','StatementService',
    'ModuleMetadata','webview','SecureElement','ContactsPicker','VirtualDeviceManager',
    'CrossDeviceSync','EyeDropper','Telecom','TelecomServiceResources','TelecomShim','TelecomUi',
    'TelephonyProvider','TelephonyProviderHsum','UsbDisableDebugger','aisealhostservice',
    'AppSearchAiSealConfig','CameraExtensionsProxy','MusicFX',
])
_cat('Java库/JNI', [
    'android.hidl.base-V1.0-java','android.hidl.manager-V1.0-java','android.test.base',
    'android.test.mock','android.test.runner','com.android.future.usb.accessory',
    'com.android.location.provider','com.android.media.remotedisplay',
    'com.android.media.remotedisplay.xml','com.android.mediadrm.signer','com.android.nfc_extras',
    'com.android.nfcservices','ext','ims-common','javax.obex','org.apache.http.legacy',
    'services','telephony-common','voip-common','framework-minus-apex-install-dependencies',
    'dex_bootjars','androidx.window.extensions','androidx.window.sidecar',
    'com.android.extensions.appfunctions','appfunctions.extension.xml',
    'gsi_overlay_framework','gsi_overlay_systemui','frameworks-base-overlays',
    'frameworks-base-overlays-debug','com.android.hardware.biometrics.fingerprint.virtual',
    'libandroid_runtime','libandroid_servers','libandroidfw','libalarm_jni',
    'libaudioeffect_jni','libdrmframework_jni','libmedia_jni','libmonkey_jni','librs_jni',
    'librtp_jni','libvintf_jni','libwebviewchromium_loader','libwebviewchromium_plat_support',
    'framework-ondeviceintelligence-platform','framework-telecom','framework-platformtelephony',
    'framework-conscrypt-nsc','framework-network-security-config',
])
_cat('ART/zygote', [
    'bcc','cppreopts.sh','otapreopt_script','dirty-image-objects','preloaded-classes',
    'hiddenapi-package-whitelist.xml','idmap2','idmap2d','odsign','ld.mc','app_process',
    'init.zygote32.rc','init.zygote64.rc','init.zygote64_32.rc','zygote_next',
])
_cat('Java框架配套服务(PLAN裁掉)', [
    'vold','installd','keystore2','credstore','gatekeeperd','incident','incidentd',
    'incident_helper','incident-helper-cmd','netd','ndc','dnsmasq','mdnsd','sdcard',
    'monkey','uiautomator',
])
_cat('Java/服务apex', [
    'aosp_mainline_modules','com.android.apex.cts.shim.v1_prebuilt','com.android.cellbroadcast',
    'com.android.crashrecovery','com.android.profiling','com.android.telephonycore',
    'com.android.webapp',
])
_cat('显示栈', [
    'surfaceflinger','bootanimation','screencap','screenrecord','blank_screen','sfdo','libgui',
    'libEGL','libEGL_angle','libGLESv1_CM','libGLESv1_CM_angle','libGLESv2','libGLESv2_angle',
    'libGLESv3','libinputflinger',
])
# 'VNDK版本垫片' 类目已撤销（2026-09-30）：init.gsi.rc/init.vndk-nodef.rc 保留官方模块，
# nodef 行为由平台补丁 0016 改为告警。见 TRIM-LEDGER.md「GSI版本探测rc垫片」节。
_cat('PM/framework元数据XML', [
    'app-lock-exempt.xml','approved-ogki-builds.xml','enhanced-confirmation.xml',
    'kernel-lifetimes.xml','initial-package-stopped-states.xml','package-shareduid-allowlist.xml',
    'platform.xml','privapp-permissions-platform.xml','preinstalled-packages-asl-files.xml',
    'preinstalled-packages-media-system.xml','preinstalled-packages-platform-generic-system.xml',
    'preinstalled-packages-platform-handheld-system.xml',
    'preinstalled-packages-platform-telephony-system.xml','preinstalled-packages-platform.xml',
    'preinstalled-packages-strict-signature.xml','android.software.credentials.prebuilt.xml',
    'android.software.webview.prebuilt.xml','android.software.window_magnification.prebuilt.xml',
    'framework-audio_effects.xml','framework-graphics','framework-location','framework-sysconfig.xml',
    'preinstalled-packages-handheld-system-ext.xml','preinstalled-packages-gsi-system-ext.xml',
    'preinstalled-packages-media-system-ext.xml','preinstalled-packages-media-product.xml',
    'preinstalled-packages-base-product.xml','apns-full-conf.xml',
    'android.software.preview_sdk.prebuilt.xml',
])
_cat('AVF虚拟化(GSI继承,picodroid无VM需求)', ['com.android.compos'])
# VNDK v31-34 曾列此处，2026-09-30 用户裁定"必须全包含"恢复，不再是删除项。

# 保留 apex（用户已拍板：apexd + 最小原生集；com.android.runtime=bionic支撑非ART）
KEEP_APEX = ['com.android.adbd','com.android.i18n','com.android.tzdata','com.android.resolv',
             'com.android.media','com.android.media.swcodec','com.android.neuralnetworks',
             'com.android.runtime']

DEP_LINE = re.compile(r'^(\s*)"([^"]+)",(\s*(?://.*)?)$')

def balanced(text, start, open_ch, close_ch):
    depth = 0; j = start
    while j < len(text):
        if text[j] == open_ch: depth += 1
        elif text[j] == close_ch:
            depth -= 1
            if depth == 0: return j
        j += 1
    raise ValueError('unbalanced')

def find_block(text, name):
    for m in re.finditer(r'([a-z_0-9]+)\s*\{', text):
        if m.group(1) == 'soong_config_module_type': continue
        s = text.index('{', m.start()); e = balanced(text, s, '{', '}')
        blk = text[m.start():e+1]
        if re.search(r'^\s*name:\s*"' + re.escape(name) + r'",?\s*$', blk, re.M):
            return blk
    raise SystemExit('missing ' + name)

def dep_expr_span(text, kw):
    """从 kw('deps:'/'required:') 起，吃掉数组 + 全部链式 + select(...) 续接。返回 (span_text, end)"""
    m = re.search(kw + r':\s*\[', text)
    if not m: return None, 0
    s = text.index('[', m.start()); e = balanced(text, s, '[', ']')
    end = e + 1
    while True:
        m2 = re.compile(r'\s*\+\s*select\(').match(text, end)
        if not m2: break
        p = text.index('(', m2.start()); e2 = balanced(text, p, '(', ')')
        end = e2 + 1
    return text[s:end], end

def remove_dep_lines(seg):
    kept, removed = [], []
    for ln in seg.split('\n'):
        m = DEP_LINE.match(ln)
        if m and m.group(2) in R:
            removed.append((m.group(2), R[m.group(2)])); continue
        kept.append(ln)
    return '\n'.join(kept), removed

def drop_empty_selects(seg):
    """删除（删除条目后）不含任何行式条目的 + select(...) 组。"""
    out = seg
    while True:
        changed = False
        for m in re.finditer(r'\s*\+\s*select\(', out):
            p = out.index('(', m.start()); e = balanced(out, p, '(', ')')
            group = out[m.start():e+1]
            if not re.search(DEP_LINE.pattern, group, re.M):
                out = out[:m.start()] + out[e+1:]
                changed = True
                break
        if not changed: return out

def process_module(block, new_name):
    total_removed = []
    out = []
    i = 0
    while True:
        span, end = dep_expr_span(block[i:], 'deps')
        if not span: break
        abs_end = i + end
        out.append(block[i:abs_end - len(span)])
        newspan, rm = remove_dep_lines(span)
        newspan = drop_empty_selects(newspan)
        total_removed += rm
        out.append(newspan)
        i = abs_end
    out.append(block[i:])
    nb = ''.join(out)
    old = re.search(r'name:\s*"([^"]+)"', block).group(1)
    nb = nb.replace(f'name: "{old}"', f'name: "{new_name}"', 1)
    return nb, total_removed

generic = open(GENERIC_BP).read()
gsi = open(GSI_BP).read()

def extract_var(text, varname):
    m = re.search(r'^' + varname + r'\s*=\s*\[', text, re.M)
    s = text.index('[', m.start()); e = balanced(text, s, '[', ']')
    return text[m.start():e+1]

parts = []
parts.append('''// picodroid system image —— 复制自 android17 GSI 镜像定义后按台账删减。
// 原文来源：
//   build/make/target/product/generic/Android.bp  (system/system_ext/product_image_defaults)
//   build/make/target/product/gsi/Android.bp      (android_gsi_defaults / android_gsi)
// 纪律：除台账（TRIM-LEDGER.md）删除项与改名外全部保持原文；分区姿态与 GSI 完全一致：
//   - 顶层 /system_ext、/product 为符号链 → /system/ 内部（GSI 单一自足 system 姿态）
//   - skip_mount.cfg 跳过 product/system_ext/oem 挂载（沿用 GSI 原模块 gsi_skip_mount.cfg）
//   - /vendor、/odm 等为真实目录挂载点（generic_rootdirs）
package {
    default_applicable_licenses: ["Android-Apache-2.0"],
}

''')
parts.append('// ===== 姿态变量（原文复制）=====\n')
parts.append(extract_var(generic, 'generic_rootdirs') + '\n\n')
parts.append(extract_var(generic, 'generic_symlinks') + '\n\n')
parts.append(extract_var(gsi, 'gsi_symlinks') + '\n\n')

ledger = []
for src_text, old, new, note in [
    (generic, 'system_image_defaults', 'picodroid_system_image_defaults', 'system 镜像基础'),
    (generic, 'system_ext_image_defaults', 'picodroid_system_ext_image_defaults', 'system_ext 内容'),
    (generic, 'product_image_defaults', 'picodroid_product_image_defaults', 'product 内容'),
    (gsi, 'android_gsi_defaults', 'picodroid_gsi_defaults', 'GSI 姿态层'),
]:
    nb, rm = process_module(find_block(src_text, old), new)
    if old == 'system_image_defaults':
        # 原类型 system_image_defaults 是 generic/Android.bp 内的 soong_config_module_type 包装，
        # 外部文件引用存在解析顺序问题；包装分支已平铺，直接用底层类型
        nb = nb.replace('system_image_defaults {', 'android_filesystem_defaults {', 1)
    ledger.append((old, rm))
    parts.append(f'// ===== {note}（原文复制 {old} 后按台账删减）=====\n')
    if old == 'system_image_defaults':
        # soong_config TARGET_ADD_ROOT_EXTRA_VENDOR_SYMLINKS 在通用 64 位目标恒走 default 分支，
        # 平铺为 default 取值（generic_symlinks + plat_file_contexts）——台账记录
        m = re.search(r'    soong_config_variables:\s*\{', nb)
        s = nb.index('{', m.start()); e = balanced(nb, s, '{', '}')
        # 吃掉外层属性尾部逗号
        tail = nb[e+1:]
        nb = nb[:m.start()] + '    file_contexts: ":plat_file_contexts",' + tail
        # 删除因平铺而残留的孤儿闭括号行（原 soong_config 块的外层 },）
        nb = nb.replace('file_contexts: ":plat_file_contexts",,', 'file_contexts: ":plat_file_contexts",')
        nb = re.sub(r'    file_contexts: ":plat_file_contexts",\n\s*\},\n\s*\},\n',
                    '    file_contexts: ":plat_file_contexts",\n', nb, count=1)
        # 追加保留 apex（替代已删除的 aosp_mainline_modules）
        nb = nb.replace('    deps: [\n        "abx",',
                        '    deps: [\n'
                        '        // picodroid: 替代已删除的 aosp_mainline_modules（决策见 TRIM-LEDGER.md）\n'
                        + ''.join(f'        "{a}",\n' for a in KEEP_APEX)
                        + '        "abx",', 1)
    if old == 'android_gsi_defaults':
        for a, b in [('"system_image_defaults"', '"picodroid_system_image_defaults"'),
                     ('"system_ext_image_defaults"', '"picodroid_system_ext_image_defaults"'),
                     ('"product_image_defaults"', '"picodroid_product_image_defaults"')]:
            nb = nb.replace(a, b)
    parts.append(nb + '\n\n')

parts.append('''// ===== picodroid system.img（对应 android_gsi；ext4，GSI 同型）=====
android_system_image {
    name: "picodroid_system",
    defaults: ["picodroid_gsi_defaults"],
}
''')

os.makedirs(OUT, exist_ok=True)
open(f'{OUT}/Android.bp', 'w').write(''.join(parts))

# ================= make 侧镜像 =================
def parse_dep_entries(block):
    """迭代块内全部 deps 表达式（含 multilib/arch 嵌套）。
    返回 (plain, debug, arm64_only)：arm64_only=arch.arm64 段内条目。"""
    plain, debug, arm64_only = [], [], []
    i = 0
    while True:
        m = re.compile(r'deps:\s*\[').search(block, i)
        if not m: break
        s = block.index('[', m.start()); e = balanced(block, s, '[', ']')
        end = e + 1
        # arch.arm64 上下文探测（向前 120 字符内出现 arm64: 且不在 multilib 内）
        ctx = block[max(0, m.start()-120):m.start()]
        in_arm64 = re.search(r'arm64:\s*\{[^{}]*$', ctx) and not re.search(r'multilib:\s*\{[^{}]*$', ctx)
        while True:
            m2 = re.compile(r'\s*\+\s*select\(').match(block, end)
            if not m2: break
            p = block.index('(', m2.start()); e2 = balanced(block, p, '(', ')')
            group = block[m2.start():e2+1]
            is_debug = 'product_variable("debuggable")' in group
            branch = None
            for ln in group.split('\n'):
                bm = re.match(r'\s*(true|default):\s*\[', ln)
                if bm: branch = bm.group(1); continue
                dm = DEP_LINE.match(ln)
                if dm and dm.group(2) not in R:
                    tgt = debug if is_debug else plain
                    tgt.append(dm.group(2))
            end = e2 + 1
        for x in re.findall(r'"([^"]+)"', block[s:e+1]):
            if x in R: continue
            (arm64_only if in_arm64 else plain).append(x)
        i = end
    return plain, debug, arm64_only

p1, d1, a1 = parse_dep_entries(find_block(generic, 'system_image_defaults'))
p2, d2, a2 = parse_dep_entries(find_block(generic, 'system_ext_image_defaults'))
p3, d3, a3 = parse_dep_entries(find_block(generic, 'product_image_defaults'))
p4, d4, a4 = parse_dep_entries(find_block(gsi, 'android_gsi_defaults'))

def dedup(xs): return list(dict.fromkeys(xs))
_mirror_strip = {'omapi', 'mediametrics', 'uprobestats', 'libuprobestats_client'}
pkg_common = dedup([n for n in p1 + KEEP_APEX + p2 + p3 + p4 if n not in _mirror_strip] + ['frameworks_sounds'])
pkg_arm64 = dedup(a1 + a2 + a3 + a4)
pkg_debug = dedup(d1 + d2 + d3 + d4)
dbg_arm64 = []

def fmt_mk_list(names, per_line=6):
    lines = []
    for i in range(0, len(names), per_line):
        chunk = names[i:i+per_line]
        lines.append('    ' + ' \\\n'.join(chunk) + ' \\')
    return '\n'.join(lines)

mk = f'''# picodroid 公共产品配置
# 复制自 build/make/target/product/gsi_release.mk 后按台账删减（TRIM-LEDGER.md），
# 并附 soong 镜像 deps 的 make 侧镜像（file_list_diff 一致性由构建校验）。

PRODUCT_ARTIFACT_PATH_REQUIREMENT_ALLOWED_LIST += \\
    system/etc/init/config \\
    system/product/% \\
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

PRODUCT_PRODUCT_PROPERTIES += \\
    ro.crypto.metadata_init_delete_all_keys.enabled=false \\
    debug.codec2.bqpool_dealloc_after_stop=1 \\

# GSI 分区姿态：skip_mount.cfg（沿用 GSI 原模块）+ HAL 存活配套（gsi_release 原文）
PRODUCT_PACKAGES += \\
    gsi_skip_mount.cfg \\
    hwservicemanager \\
    android.hidl.allocator@1.0-service \\
    android.hidl.memory@1.0-impl \\
    wificond \\

# ===== soong 镜像 deps 的 make 侧镜像（与 Android.bp 保持锁步；差异由 file_list_diff 报告）=====
PRODUCT_PACKAGES += \\
{fmt_mk_list(pkg_common)}

PRODUCT_PACKAGES_DEBUG += \\
{fmt_mk_list(pkg_debug)}
'''
def _norm_lists(text):
    lines = text.split('\n'); out = []; in_list = False
    for ln in lines:
        if re.match(r'^\w[\w.]*\s*\+=\s*\\$', ln):
            in_list = True; out.append(ln); continue
        if in_list:
            s = ln.strip()
            if s == '':
                if out and out[-1].endswith('\\'): out[-1] = out[-1][:-1].rstrip()
                in_list = False; out.append(ln); continue
            name = s.rstrip('\\').strip()
            if name and not name.startswith('#'):
                out.append('    ' + name + ' \\'); continue
            out.append(ln); continue
        out.append(ln)
    if out and out[-1].endswith('\\'): out[-1] = out[-1][:-1].rstrip()
    return '\n'.join(out)
open(f'{OUT}/picodroid_common.mk', 'w').write(_norm_lists(mk))

arch_x = '''# anland_picodroid_x86_64 —— Cuttlefish 日常迭代产品
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
'''
arch_a = '''# anland_picodroid_arm64 —— 真机 GSI 产品（与 x86_64 同步维护）
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

# arm64 专属（hwasan 支持，GSI 原有）
PRODUCT_PACKAGES += \\
{arm64}
PRODUCT_PACKAGES_DEBUG += \\
{dbg}
'''.format(arm64=' \\\n    '.join(pkg_arm64) + ' \\', dbg=' \\\n    '.join(dbg_arm64) + ' \\')
open(f'{OUT}/picodroid_x86_64.mk', 'w').write(arch_x)
open(f'{OUT}/picodroid_arm64.mk', 'w').write(arch_a.replace('{arm64}', fmt_mk_list(pkg_arm64)).replace('{dbg}', fmt_mk_list(dbg_arm64)))

bc_x = '''# picodroid x86_64 BoardConfig —— 复制自 device/generic/x86_64 后删 32 位次架构
TARGET_NO_BOOTLOADER := true
TARGET_NO_KERNEL := true
TARGET_CPU_ABI := x86_64
TARGET_ARCH := x86_64
TARGET_ARCH_VARIANT := x86_64

SMALLER_FONT_FOOTPRINT := true
MINIMAL_FONT_FOOTPRINT := true
BUILD_EMULATOR := false
BOARD_HAVE_BLUETOOTH := true
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/generic/common/bluetooth

TARGET_USERIMAGES_USE_EXT4 := true
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 2147483648
BOARD_USERDATAIMAGE_PARTITION_SIZE := 576716800
BOARD_CACHEIMAGE_PARTITION_SIZE := 69206016
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_FLASH_BLOCK_SIZE := 512
TARGET_USERIMAGES_SPARSE_EXT_DISABLED := true

# VNDK 与平台同版（PLAN §3；真机阶段对齐目标 vendor）
BOARD_VNDK_VERSION := current
'''
bc_a = '''# picodroid arm64 BoardConfig —— 复制自 device/generic/arm64 后删 32 位次架构
TARGET_NO_BOOTLOADER := true
TARGET_NO_KERNEL := true

TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_VARIANT := generic
TARGET_CPU_ABI := arm64-v8a

SMALLER_FONT_FOOTPRINT := true
MINIMAL_FONT_FOOTPRINT := true
BOARD_HAVE_BLUETOOTH := true
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/generic/common/bluetooth

BOARD_USES_GENERIC_AUDIO := true
USE_CAMERA_STUB := true

TARGET_USERIMAGES_USE_EXT4 := true
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 2147483648
BOARD_USERDATAIMAGE_PARTITION_SIZE := 576716800
BOARD_CACHEIMAGE_PARTITION_SIZE := 69206016
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_FLASH_BLOCK_SIZE := 512
TARGET_USERIMAGES_SPARSE_EXT_DISABLED := true

BOARD_VNDK_VERSION := current
'''
os.makedirs(f'{OUT}/x86_64', exist_ok=True); os.makedirs(f'{OUT}/arm64', exist_ok=True)
open(f'{OUT}/x86_64/BoardConfig.mk', 'w').write(bc_x)
open(f'{OUT}/arm64/BoardConfig.mk', 'w').write(bc_a)

open(f'{OUT}/AndroidProducts.mk', 'w').write('''PRODUCT_MAKEFILES := \\
    $(LOCAL_DIR)/picodroid_arm64.mk \\
    $(LOCAL_DIR)/picodroid_x86_64.mk
''')

# ================= 台账文件 =================
bycat = {}
for name, cat in R.items(): bycat.setdefault(cat, []).append(name)
led = ['# picodroid 裁剪台账（GSI → picodroid）',
       '',
       '> 生成于 ' + subprocess.run(['date','-Iseconds'],capture_output=True,text=True).stdout.strip(),
       '> 纪律：复制 build/make/target/product/{generic,gsi}/Android.bp 四个镜像 defaults 后仅做下列删除；',
       '> 每处删除可追溯到类目。凡不在本表者一律原文保留（有节制裁剪）。', '']
order = ['Java应用','Java库/JNI','ART/zygote','Java框架配套服务(PLAN裁掉)','Java/服务apex','显示栈','PM/framework元数据XML','VNDK版本垫片','AVF虚拟化(GSI继承,picodroid无VM需求)']
for cat in order:
    names = sorted(set(bycat.get(cat, [])))
    led.append(f'## {cat}（{len(names)}）')
    led.append(', '.join(f'`{n}`' for n in names))
    led.append('')
led += ['## 结构性调整（非删除，逐项说明）', '''
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
   nodef 的 `reboot bootloader` 改为 kmsg 告警（picodroid 跑 current-vendor）。
   双轨：build/make 本地提交 + patches/0016-*.patch。
4. VNDK v31–v34 apex：曾删；2026-09-30 用户裁定"必须全包含"恢复（对齐官方 GSI 通刷能力）。
5. make 侧不继承 generic_system.mk / gsi_system_ext.mk / gsi_product.mk / updatable_apex.mk /
   core_64_bit(_only).mk（Java 产品链与 zygote/apex-shim 混入），改以 picodroid_common.mk
   镜像 soong deps；PRODUCT_PACKAGES 与 Android.bp deps 锁步，由 file_list_diff 校验闭环。
6. 平台补丁 0001（system/core/rootdir）：init.rc 的 required 去除 platform-bootclasspath 与
   boringssl-zygote rc；init.rc 正文删 zygote import 两行与 conscrypt-apex boringssl 自测服务块。
   双轨：system/core 本地提交 + patches/0001-*.patch。
7. Wi-Fi/BT：无 NDK 面（public.libraries.android.txt 已核实）；com.android.wifi/bt apex 为
   system_server 侧 Java 栈，删除不影响 vendor HAL 存活（M2 验证）。
''']
open(f'{OUT}/TRIM-LEDGER.md', 'w').write('\n'.join(led))

# ================= init.rc fork（写入设备树，由平台补丁引用安装）=================
init_rc = open(f'{ROOTDIR}/init.rc').read()
fork = init_rc
fork = fork.replace('import /system/etc/init/hw/init.${ro.zygote}.rc\n', '')
fork = fork.replace('import /system/etc/init/hw/init.boringssl.${ro.zygote}.rc\n', '')
# 删除 conscrypt-apex boringssl 自测服务块（conscrypt apex 已删，exec 必失败）
for svc in ['boringssl_self_test_apex32', 'boringssl_self_test_apex64']:
    pat = re.compile(r'service ' + svc + r' \S+\n(?:    .*\n|\n)*?(?=\n|\Z)')
    m = re.search(r'^service ' + svc + r' .*$\n((?:    .*\n)*)', fork, re.M)
    if m:
        fork = fork[:m.start()] + fork[m.end():]
os.makedirs(f'{OUT}/rootdir', exist_ok=True)
open(f'{OUT}/rootdir/init.picodroid.rc', 'w').write(
    '# picodroid init.rc —— 复制自 system/core/rootdir/init.rc 后按台账删减\n'
    '# （删 zygote/boringssl-zygote import、conscrypt-apex boringssl 自测服务；其余原文）\n'
    + fork)

json.dump({'pkg_common': pkg_common, 'pkg_debug': pkg_debug,
           'pkg_arm64': pkg_arm64, 'removed': {k: v for k, v in R.items()}},
          open('/tmp/picodroid_state.json', 'w'), indent=1)

total = sum(len(r) for _, r in ledger)
print(f'Android.bp modules: 4+1; removed entries: {total}')
print(f'mirror: common={len(pkg_common)} debug={len(pkg_debug)} arm64_extra={len(pkg_arm64)}')
for old, rm in ledger:
    print(f'  {old}: -{len(rm)}')
