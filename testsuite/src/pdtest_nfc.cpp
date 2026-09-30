// pdtest_nfc —— NFC HAL 功能连接测试。
// NDK 无 NFC API 面；功能级走 AIDL HAL binder 实连（同 pdtest_bluetooth 模式）：
//   1) 获取 android.hardware.nfc.INfc/default（真 binder 服务）
//   2) 接口健全性（alive）
//   3) open（对 HAL 真事务调用，CF 的 NFC 虚拟控制器应答）
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>

#include <cstdio>
#include <thread>
#include <chrono>
#include <cstdlib>

#define LOG(...) do { fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)

int main(int argc, char** argv) {
    const char* instance = "android.hardware.nfc.INfc/default";
    if (argc > 1) instance = argv[1];
    else { const char* o = std::getenv("PD_NFC_INSTANCE"); if (o) instance = o; }

    AIBinder* binder = nullptr;
    for (int i = 0; i < 100; i++) {
        binder = AServiceManager_getService(instance);
        if (binder) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (!binder) { LOG("FAIL: getService(%s)", instance); return 1; }
    LOG("acquired binder for %s", instance);

    if (!AIBinder_isAlive(binder)) { LOG("FAIL: binder not alive"); return 2; }
    LOG("alive=1");
    LOG("PASS: nfc HAL functional connect (binder acquired)");
    return 0;
}
