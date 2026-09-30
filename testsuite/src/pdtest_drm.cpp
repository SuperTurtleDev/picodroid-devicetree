// pdtest_drm —— DRM (MediaDrm) NDK 功能测试。
// 判据：AMediaDrm 构造 clearkey UUID（isCryptoSchemeSupported 先行判定）+ 创建 media session。
// clearkey 是镜像内注册的原生 DRM 实现（service 列表 android.hardware.drm.IDrmFactory/clearkey）。
#include <media/NdkMediaDrm.h>

#include <cstdio>
#include <cstring>
#include <cstdint>

#define LOG(...) do { fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)

// EDEF8BA9-79D6-4ACE-A3C8-27DCD51D21ED (clearkey)
static const uint8_t kClearKeyUUID[16] = {
    0xED, 0xEF, 0x8B, 0xA9, 0x79, 0xD6, 0x4A, 0xCE,
    0xA3, 0xC8, 0x27, 0xDC, 0xD5, 0x1D, 0x21, 0xED
};

int main() {
    // 1) 算法支持判定（真调用 mediaserver DRM 栈；mimeType 可空=任意）
    bool supported = AMediaDrm_isCryptoSchemeSupported(&kClearKeyUUID[0], nullptr);
    LOG("clearkey supported: %d", supported ? 1 : 0);
    if (!supported) { LOG("FAIL: clearkey scheme unsupported"); return 1; }

    // 2) DRM 实例构造
    AMediaDrm* drm = AMediaDrm_createByUUID(&kClearKeyUUID[0]);
    if (!drm) { LOG("FAIL: AMediaDrm_createByUUID"); return 2; }
    LOG("AMediaDrm created");

    // 3) 会话创建（真密钥会话，clearkey 本地实现）
    AMediaDrmSessionId session = {};
    media_status_t st = AMediaDrm_openSession(drm, &session);
    if (st != AMEDIA_OK) { LOG("FAIL: openSession %d", st); AMediaDrm_release(drm); return 3; }
    LOG("session opened (id len %d)", session.length);

    AMediaDrm_closeSession(drm, &session);
    AMediaDrm_release(drm);
    LOG("PASS: drm NDK (clearkey scheme + session lifecycle)");
    return 0;
}
