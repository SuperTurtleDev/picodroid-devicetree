// pdtest_audio_ll —— AAudio 低延迟输出路径 + 音频线程注册（requestPriority）回归。
// 防回归 1：requestStart 一路走到 AudioFlinger/MMAP 时曾因 SchedulingPolicy
// requestPriority 死循环永久挂死——open+start+写块整体用 alarm(8) 兜底。
// 判据 1：8s 内成功写入 >=10 个 96 帧块（极小 buffer 逼 FAST/MMAX 路径）、
// xruns 有记录、流状态非 DISCONNECTED（STARTED/FLUSHING 等均接受）。
// 防回归 2：音频线程注册路径。注：本树 NDK 已移除 AAudioStream_registerAudioThread
// （libaaudio.so 无此导出符号），该路径现在由带 data callback 的流在启动时于
// 新建回调线程内自动触发：wrapUserThread() -> registerThread() -> binder
// registerAudioThread -> audioserver requestPriority(SchedulingPolicy)——正是
// 历史死循环点。故第二阶段用独立 callback 流：requestStart 后新建回调线程先做
// 线程注册再进回调，判据 2 = 3s 内回调被调用 >=1 次（即注册已返回、未挂死）——
// 这是 NDK 规范行为（AAudio 数据回调流必须可用），平台侧需保证注册路径成功
// （picodroid 无 system_server 时 requestPriority 为 best-effort，不得拒绝，
// 见 mediautils/SchedulingPolicyService.cpp 平台补丁）。该段用 alarm(3) 兜底；
// 回调首轮即返回 STOP，顺带覆盖 unregisterThread 路径。
#include <aaudio/AAudio.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>

#define LOG(...) do { fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)

static void on_alarm(int) {
    static const char msg[] =
        "FAIL: pdtest_audio_ll timed out -- AAudio low-latency/thread-registration "
        "path hung (SchedulingPolicy requestPriority regression?)\n";
    ssize_t rc = write(2, msg, sizeof(msg) - 1);
    (void)rc;
    _exit(9);
}

static std::atomic<int> g_cb_calls{0};
static std::atomic<pid_t> g_cb_tid{0};

// 回调首轮记 tid 后即 STOP：wrapUserThread 随后走 unregisterThread() 收尾
static aaudio_data_callback_result_t on_data(AAudioStream*, void*, void*, int32_t) {
    g_cb_tid = (pid_t)syscall(SYS_gettid);
    g_cb_calls++;
    return AAUDIO_CALLBACK_RESULT_STOP;
}

// 低延迟输出的公共 builder 参数（写流与回调流共用）
static void set_ll_params(AAudioStreamBuilder* b, int32_t dir) {
    AAudioStreamBuilder_setDirection(b, dir);
    AAudioStreamBuilder_setPerformanceMode(b, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setFormat(b, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setChannelCount(b, 1);
    AAudioStreamBuilder_setSampleRate(b, 48000);
}

int main() {
    signal(SIGALRM, on_alarm);

    // ---- 阶段 1：LOW_LATENCY 写流（alarm(8) 包裹 open/start/写块）----
    alarm(8);
    AAudioStream* s = nullptr;
    auto open_ll = [&s](bool tiny_capacity) -> aaudio_result_t {
        AAudioStreamBuilder* b = nullptr;
        if (AAudio_createStreamBuilder(&b) != AAUDIO_OK || !b) return AAUDIO_ERROR_INTERNAL;
        set_ll_params(b, AAUDIO_DIRECTION_OUTPUT);
        // 极小 buffer 逼 FAST/MMAP 路径（builder 只有 capacity；实际 size 在 open 后收紧）
        if (tiny_capacity) AAudioStreamBuilder_setBufferCapacityInFrames(b, 96);
        aaudio_result_t r = AAudioStreamBuilder_openStream(b, &s);
        AAudioStreamBuilder_delete(b);
        return r;
    };
    aaudio_result_t r = open_ll(true);
    if (r != AAUDIO_OK) {
        // 个别器件 burst > 96 时极小 capacity 可能 open 失败：回退默认 capacity，
        // LOW_LATENCY 属性不变，open 后仍把 size 收到 96
        LOG("note: open with capacity=96 failed (%d), retrying default capacity", r);
        r = open_ll(false);
    }
    if (r != AAUDIO_OK || !s) { LOG("FAIL: open LL output stream %d", r); return 2; }
    if (AAudioStream_setBufferSizeInFrames(s, 96) != AAUDIO_OK)
        LOG("note: setBufferSizeInFrames(96) not honored");
    LOG("LL stream: rate=%d burst=%d bufsize=%d/%d",
        AAudioStream_getSampleRate(s), AAudioStream_getFramesPerBurst(s),
        AAudioStream_getBufferSizeInFrames(s), AAudioStream_getBufferCapacityInFrames(s));

    if (AAudioStream_requestStart(s) != AAUDIO_OK) { LOG("FAIL: start LL stream"); AAudioStream_close(s); return 3; }

    const int32_t kBlock = 96;  // 96 帧 @48k = 2ms
    std::vector<int16_t> buf(kBlock, 0);
    int writes = 0;
    auto wdl = std::chrono::steady_clock::now() + std::chrono::seconds(6);
    while (writes < 10 && std::chrono::steady_clock::now() < wdl) {
        aaudio_result_t n = AAudioStream_write(s, buf.data(), kBlock, 100000000LL /*100ms*/);
        if (n == kBlock) writes++;
        else if (n < 0) { LOG("write err %d", n); break; }
    }
    int32_t xruns = AAudioStream_getXRunCount(s);
    aaudio_stream_state_t state = AAudioStream_getState(s);
    bool state_ok = (state != AAUDIO_STREAM_STATE_DISCONNECTED);
    LOG("writes=%d/10 xruns=%d state=%d perf=%d", writes, xruns, (int)state,
        (int)AAudioStream_getPerformanceMode(s));
    AAudioStream_requestStop(s);
    AAudioStream_close(s);
    alarm(0);
    if (writes < 10) { LOG("FAIL: only %d/10 blocks written", writes); return 4; }
    if (!state_ok) { LOG("FAIL: stream disconnected (state=%d)", (int)state); return 5; }

    // ---- 阶段 2：回调流线程注册路径（alarm(3) 包裹 start/回调/unregister/close）----
    alarm(3);
    AAudioStreamBuilder* b2 = nullptr;
    if (AAudio_createStreamBuilder(&b2) != AAUDIO_OK || !b2) { LOG("FAIL: createStreamBuilder(cb)"); return 6; }
    set_ll_params(b2, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setDataCallback(b2, on_data, nullptr);
    AAudioStreamBuilder_setFramesPerDataCallback(b2, kBlock);
    AAudioStream* s2 = nullptr;
    r = AAudioStreamBuilder_openStream(b2, &s2);
    AAudioStreamBuilder_delete(b2);
    if (r != AAUDIO_OK || !s2) { LOG("FAIL: open callback stream %d", r); alarm(0); return 7; }
    auto t0 = std::chrono::steady_clock::now();
    if (AAudioStream_requestStart(s2) != AAUDIO_OK) {
        LOG("FAIL: start callback stream"); AAudioStream_close(s2); alarm(0); return 8;
    }
    // 等回调首轮（软期限 2.5s，先于 alarm(3) 给出干净 FAIL）；回调线程注册挂死/
    // 被平台拒绝（注册失败会被 wrapUserThread 转 DISCONNECTED，回调永不来）则 FAIL
    while (g_cb_calls == 0 &&
           std::chrono::steady_clock::now() - t0 < std::chrono::milliseconds(2500))
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    AAudioStream_requestStop(s2);
    AAudioStream_close(s2);  // 回调线程 join：unregisterThread 挂死由 alarm(3) 兜底
    alarm(0);
    if (g_cb_calls == 0) {
        LOG("FAIL: callback thread never ran within 3s "
            "(thread registration hung or denied by platform)");
        return 9;
    }
    LOG("callback first invocation after %lldms on tid %d (registration returned)",
        (long long)elapsed, (int)g_cb_tid);

    LOG("PASS: audio_ll (LOW_LATENCY %dx96-frame writes, xruns=%d, state=%d; "
        "callback-thread registration returned)", writes, xruns, (int)state);
    return 0;
}
