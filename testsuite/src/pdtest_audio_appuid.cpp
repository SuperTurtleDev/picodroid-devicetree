// pdtest_audio_appuid —— 非 root（app uid 10086）音频权限恒真测试。
// 防回归：uid 10086 下 AAudio 输入流 open 曾 EX_ILLEGAL_STATE("controller never
// populated")；输出流 open 曾因 OP_PLAY_AUDIO appop 交互停顿 ~10s 后静音。
// 判据：uid 10086 下 录音 open 成功且读到帧数 > 0；播放 open 成功且 5s 内完成，
// 1s 静音写入有产出。
// 运行形态：getuid()!=0 时直接跑流程（天然 app 环境）；root 下 fork 子进程，
// 先 setgid(10086) 再 setuid(10086)（顺带清空补充组）后在子进程跑——fork 前
// 不做任何 AAudio 调用，动态库已随 exec 加载，setuid 后纯函数调用无碍；
// 子进程 stderr 继承，父进程 waitpid 取子退出码为测试结果。
#include <aaudio/AAudio.h>

#include <unistd.h>
#include <grp.h>
#include <sys/wait.h>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <chrono>
#include <vector>

#define LOG(...) do { fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)

static const int kAppUid = 10086;

static void on_alarm(int) {
    static const char msg[] =
        "FAIL: pdtest_audio_appuid timed out -- audio path hung under app uid\n";
    ssize_t rc = write(2, msg, sizeof(msg) - 1);
    (void)rc;
    _exit(9);
}

static int32_t timed_open(int32_t dir, AAudioStream** out, int64_t* ms) {
    AAudioStreamBuilder* b = nullptr;
    if (AAudio_createStreamBuilder(&b) != AAUDIO_OK || !b) return AAUDIO_ERROR_INTERNAL;
    AAudioStreamBuilder_setDirection(b, dir);
    AAudioStreamBuilder_setFormat(b, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setChannelCount(b, 1);
    auto t0 = std::chrono::steady_clock::now();
    aaudio_result_t r = AAudioStreamBuilder_openStream(b, out);
    *ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    AAudioStreamBuilder_delete(b);
    return r;
}

// 在当前（应为 10086）uid 下跑最小录音 1s + 播放 1s 流程；0=PASS
static int app_flow() {
    LOG("running audio flow as uid=%d", getuid());
    alarm(30);  // 兜底：历史 appop 停顿/挂死不得拖死整个套件

    // 1) 录音 1s：open 成功且读到帧数 > 0（历史：EX_ILLEGAL_STATE "controller
    //    never populated"）
    AAudioStream* in = nullptr; int64_t open_ms = 0;
    aaudio_result_t r = timed_open(AAUDIO_DIRECTION_INPUT, &in, &open_ms);
    if (r != AAUDIO_OK || !in) {
        LOG("FAIL: open input as uid %d: %d (%lldms)", getuid(), r, (long long)open_ms);
        return 1;
    }
    int32_t rate = AAudioStream_getSampleRate(in);
    if (rate <= 0) rate = 48000;
    if (AAudioStream_requestStart(in) != AAUDIO_OK) {
        LOG("FAIL: start input"); AAudioStream_close(in); return 2;
    }
    std::vector<int16_t> rbuf(rate / 4);
    int64_t frames = 0;
    auto rdl = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < rdl) {
        aaudio_result_t n = AAudioStream_read(in, rbuf.data(), (int32_t)rbuf.size(),
                                              1000000000LL /*1s*/);
        if (n > 0) frames += n;
        if (n < 0) { LOG("read err %d", n); break; }
    }
    LOG("record: rate=%d frames=%lld xruns=%d (open %lldms)", rate, (long long)frames,
        AAudioStream_getXRunCount(in), (long long)open_ms);
    AAudioStream_close(in);
    if (frames <= 0) { LOG("FAIL: no frames recorded under app uid"); return 3; }

    // 2) 播放 1s 静音：open 成功且 5s 内完成（历史：OP_PLAY_AUDIO 停顿 ~10s+静音）
    AAudioStream* out = nullptr;
    r = timed_open(AAUDIO_DIRECTION_OUTPUT, &out, &open_ms);
    if (r != AAUDIO_OK || !out) {
        LOG("FAIL: open output as uid %d: %d (%lldms)", getuid(), r, (long long)open_ms);
        return 4;
    }
    LOG("play open took %lldms (budget 5000)", (long long)open_ms);
    if (open_ms > 5000) { LOG("FAIL: play open stalled %lldms > 5s", (long long)open_ms); AAudioStream_close(out); return 5; }
    int32_t prate = AAudioStream_getSampleRate(out);
    if (prate <= 0) prate = 48000;
    if (AAudioStream_requestStart(out) != AAUDIO_OK) {
        LOG("FAIL: start output"); AAudioStream_close(out); return 6;
    }
    std::vector<int16_t> wbuf(prate / 4, 0);
    int64_t written = 0;
    auto wdl = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < wdl) {
        aaudio_result_t n = AAudioStream_write(out, wbuf.data(), (int32_t)wbuf.size(),
                                               1000000000LL);
        if (n > 0) written += n;
        if (n < 0) { LOG("write err %d", n); break; }
    }
    LOG("play: rate=%d frames=%lld", prate, (long long)written);
    AAudioStream_requestStop(out);
    AAudioStream_close(out);
    alarm(0);
    if (written <= 0) { LOG("FAIL: no frames played under app uid"); return 7; }
    LOG("PASS: app-uid audio flow (uid=%d record %lld frames, play %lld frames)",
        getuid(), (long long)frames, (long long)written);
    return 0;
}

int main() {
    signal(SIGALRM, on_alarm);

    if (getuid() != 0) {
        // 已是非 root：直接跑（天然 app 环境）
        LOG("uid=%d (non-root), running flow directly", getuid());
        return app_flow();
    }

    // root：fork 后在子进程降权再跑，父进程只负责收割退出码
    pid_t pid = fork();
    if (pid < 0) { LOG("FAIL: fork"); return 20; }
    if (pid == 0) {
        // 先清补充组（root 才可调；失败不致命），再 setgid -> setuid 完成降权
        if (setgroups(0, nullptr) != 0) LOG("note: setgroups(0) failed, supplementary groups kept");
        if (setgid(kAppUid) != 0) { LOG("FAIL: setgid(%d)", kAppUid); _exit(21); }
        if (setuid(kAppUid) != 0) { LOG("FAIL: setuid(%d)", kAppUid); _exit(22); }
        if (getuid() != kAppUid) { LOG("FAIL: still uid=%d after setuid", getuid()); _exit(23); }
        LOG("dropped to uid=%d/%d", getuid(), getgid());
        _exit(app_flow());
    }

    int st = 0;
    if (waitpid(pid, &st, 0) != pid) { LOG("FAIL: waitpid"); return 24; }
    if (WIFSIGNALED(st)) { LOG("FAIL: child killed by signal %d", WTERMSIG(st)); return 25; }
    if (!WIFEXITED(st)) { LOG("FAIL: child did not exit normally"); return 26; }
    int rc = WEXITSTATUS(st);
    if (rc != 0) { LOG("FAIL: app-uid(10086) audio flow rc=%d (details above)", rc); return rc; }
    LOG("PASS: audio_appuid (uid 10086 record+play)");
    return 0;
}
