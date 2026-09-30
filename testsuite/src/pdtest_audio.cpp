// pdtest_audio —— NDK 音频功能测试：AAudio 真录音 + 真播放，OpenSL ES 引擎建立。
// 判据：录音累计帧数 > 采样目标、播放流成功 start 且写入帧数达标。
#include <aaudio/AAudio.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <cstdio>
#include <vector>
#include <cstring>
#include <chrono>
#include <thread>
#include <atomic>

#define LOG(...) do { fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)

static bool record_seconds(int seconds, int32_t* out_rate, int64_t* out_frames, int32_t* out_xruns) {
    AAudioStreamBuilder* b = nullptr;
    if (AAudio_createStreamBuilder(&b) != AAUDIO_OK || !b) { LOG("FAIL: createStreamBuilder"); return false; }
    AAudioStreamBuilder_setDirection(b, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setFormat(b, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setChannelCount(b, 1);
    AAudioStream* s = nullptr;
    aaudio_result_t r = AAudioStreamBuilder_openStream(b, &s);
    AAudioStreamBuilder_delete(b);
    if (r != AAUDIO_OK || !s) { LOG("FAIL: open input stream %d", r); return false; }
    *out_rate = AAudioStream_getSampleRate(s);
    if (AAudioStream_requestStart(s) != AAUDIO_OK) { LOG("FAIL: start input"); AAudioStream_close(s); return false; }
    std::vector<int16_t> buf(*out_rate / 2);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    while (std::chrono::steady_clock::now() < deadline) {
        aaudio_result_t n = AAudioStream_read(s, buf.data(), buf.size(), 1000000000LL /*1s*/);
        if (n > 0) *out_frames += n;
        if (n < 0) { LOG("read err %d", n); break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    *out_xruns = AAudioStream_getXRunCount(s);
    AAudioStream_close(s);
    return true;
}

static bool play_seconds(int seconds, int32_t* out_rate, int64_t* out_frames) {
    AAudioStreamBuilder* b = nullptr;
    if (AAudio_createStreamBuilder(&b) != AAUDIO_OK || !b) { LOG("FAIL: createStreamBuilder(out)"); return false; }
    AAudioStreamBuilder_setDirection(b, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setFormat(b, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setChannelCount(b, 1);
    AAudioStream* s = nullptr;
    aaudio_result_t r = AAudioStreamBuilder_openStream(b, &s);
    AAudioStreamBuilder_delete(b);
    if (r != AAUDIO_OK || !s) { LOG("FAIL: open output stream %d", r); return false; }
    *out_rate = AAudioStream_getSampleRate(s);
    if (AAudioStream_requestStart(s) != AAUDIO_OK) { LOG("FAIL: start output"); AAudioStream_close(s); return false; }
    std::vector<int16_t> buf(*out_rate / 4);  // 静音
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    while (std::chrono::steady_clock::now() < deadline) {
        aaudio_result_t n = AAudioStream_write(s, buf.data(), buf.size(), 1000000000LL);
        if (n > 0) *out_frames += n;
        if (n < 0) { LOG("write err %d", n); break; }
    }
    AAudioStream_requestStop(s);
    AAudioStream_close(s);
    return true;
}

int main() {
    // 1) AAudio 录音 2 秒
    int32_t rrate = 0, xruns = 0; int64_t rframes = 0;
    if (!record_seconds(2, &rrate, &rframes, &xruns)) return 1;
    LOG("record: rate=%d frames=%lld xruns=%d", rrate, (long long)rframes, xruns);
    if (rframes < (int64_t)rrate) { LOG("FAIL: recorded %lld < 1s of audio", (long long)rframes); return 2; }

    // 2) AAudio 播放 1 秒静音
    int32_t prate = 0; int64_t pframes = 0;
    if (!play_seconds(1, &prate, &pframes)) return 3;
    LOG("play: rate=%d frames=%lld", prate, (long long)pframes);
    if (pframes < (int64_t)prate / 2) { LOG("FAIL: played too few frames"); return 4; }

    // 3) OpenSL ES 引擎 + 录音器建立（第二个 NDK 音频入口）
    SLObjectItf engineObj = nullptr; SLEngineItf engine = nullptr;
    slCreateEngine(&engineObj, 0, nullptr, 0, nullptr, nullptr);
    if ((*engineObj)->Realize(engineObj, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) {
        LOG("FAIL: OpenSLES realize"); return 5; }
    (*engineObj)->GetInterface(engineObj, SL_IID_ENGINE, &engine);
    LOG("OpenSLES engine realized");
    // Android 简易录音器
    SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, nullptr};
    SLDataSource audioSrc = {&loc_dev, nullptr};
    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM fmt = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_16, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSink audioSnk = {&loc_bq, &fmt};
    SLObjectItf recorderObj = nullptr;
    SLInterfaceID ids[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    SLboolean req[1] = {SL_BOOLEAN_TRUE};
    if ((*engine)->CreateAudioRecorder(engine, &recorderObj, &audioSrc, &audioSnk, 1, ids, req) != SL_RESULT_SUCCESS) {
        LOG("FAIL: CreateAudioRecorder"); return 6; }
    (*recorderObj)->Realize(recorderObj, SL_BOOLEAN_FALSE);
    SLRecordItf recorder = nullptr;
    (*recorderObj)->GetInterface(recorderObj, SL_IID_RECORD, &recorder);
    if (!recorder) { LOG("FAIL: SL_IID_RECORD"); return 7; }
    LOG("OpenSLES recorder realized");
    LOG("PASS: audio NDK (AAudio record/playback + OpenSLES)");
    return 0;
}
