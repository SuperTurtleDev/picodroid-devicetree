// pdtest_sensor —— NDK 传感器功能测试：枚举 + 事件队列真实收包。
// 判据：传感器列表非空，加速计（或列表首个可用传感器）在 5s 内收到 >=3 个事件。
#include <android/sensor.h>

#include <cstdio>
#include <cstring>
#include <chrono>
#include <thread>

#define LOG(...) do { fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)

static int g_events = 0;
static int g_looper_id = 1;

static void on_event(ASensorEvent e) {
    g_events++;
    if (g_events <= 2)
        LOG("event #%d type=%d v=(%.3f,%.3f,%.3f)", g_events, e.type, e.acceleration.x, e.acceleration.y, e.acceleration.z);
}

int main() {
    ASensorManager* mgr = ASensorManager_getInstanceForPackage("com.anland.picodroid.pdtest");
    if (!mgr) { LOG("FAIL: ASensorManager"); return 1; }

    ASensorList list = nullptr;
    int n = ASensorManager_getSensorList(mgr, &list);
    LOG("sensors: %d", n);
    if (n <= 0) { LOG("FAIL: empty sensor list"); return 2; }
    for (int i = 0; i < n && i < 6; i++)
        LOG("  [%d] %s (%s, vendor=%s)", i, ASensor_getName(list[i]),
            ASensor_getStringType(list[i]), ASensor_getVendor(list[i]));

    // 优先加速计，否则取第一个
    const ASensor* accel = ASensorManager_getDefaultSensor(mgr, ASENSOR_TYPE_ACCELEROMETER);
    const ASensor* target = accel ? accel : list[0];
    if (!target) { LOG("FAIL: no usable sensor"); return 3; }
    LOG("target sensor: %s", ASensor_getName(target));

    ALooper* looper = ALooper_forThread();
    if (!looper) looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    ASensorEventQueue* q = ASensorManager_createEventQueue(mgr, looper, g_looper_id, nullptr, nullptr);
    if (!q) { LOG("FAIL: createEventQueue"); return 4; }

    if (ASensorEventQueue_enableSensor(q, target) != 0) { LOG("FAIL: enableSensor"); return 5; }
    ASensorEventQueue_setEventRate(q, target, 100000);  // 10 Hz（受限为传感器最小速率亦视为成功）

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline && g_events < 3) {
        ASensorEvent event;
        while (ASensorEventQueue_getEvents(q, &event, 1) > 0) on_event(event);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    ASensorEventQueue_disableSensor(q, target);
    ASensorManager_destroyEventQueue(mgr, q);

    if (g_events < 3) { LOG("FAIL: only %d events in 5s", g_events); return 6; }
    LOG("PASS: sensor NDK (%d events, %d sensors)", g_events, n);
    return 0;
}
