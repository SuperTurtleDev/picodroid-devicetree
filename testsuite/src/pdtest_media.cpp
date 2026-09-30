// pdtest_media —— NDK 媒体功能测试：AMediaCodec 真实解码闭环。
// 码流：内置 BBB H264（176x144, annex-B，hardware/interfaces/media/res），
// 按起始码切 NAL 喂给 c2.android.avc.decoder；判据：解码输出帧数 >= 30 且渲染成功。
// （v1 的编码路径因 flexible 格式跨距写入导致堆损坏已移除；编码闭环待跨距正确的输入后再加。）
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>

#include "bbb_stream.h"  // genrule 生成：kStream[] / kStreamLen

// AMediaCodec 依赖 C2 服务 → 本进程的 oneway binder 回调（onInput/OutputBufferAvailable、
// onWorkDone 等）投递输入/输出 buffer。裸 NDK 进程不自动起 binder 线程池（无 ART/app 框架），
// 回调滞留 binder 驱动永不送达 → dequeue* 死等（实测卡 AMediaCodec_dequeueOutputBuffer，
// 客户端 0 个 Binder:* 线程、服务端 worker 空闲）。libbinder_ndk 的
// ABinderProcess_startThreadPool 驱动的就是 libstagefright 所用 libbinder 线程池；
// NDK sysroot 无 android/binder_process.h 头（平台专属头），此处按官方签名自行声明。
extern "C" void ABinderProcess_startThreadPool(void);

#define LOG(...) do { fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)

// annex-B 起始码切分（返回每个 NAL 的起始偏移）
static std::vector<size_t> nal_offsets(const uint8_t* d, size_t n) {
    std::vector<size_t> offs;
    for (size_t i = 0; i + 3 < n; i++) {
        if (d[i] == 0 && d[i+1] == 0 && d[i+2] == 0 && d[i+3] == 1) {
            offs.push_back(i);
            i += 3;
        } else if (d[i] == 0 && d[i+1] == 0 && d[i+2] == 1) {
            offs.push_back(i);
            i += 2;
        }
    }
    return offs;
}

int main() {
    ABinderProcess_startThreadPool();  // 见上方注释：回调可达性前置
    const char* dec_name = "c2.android.avc.decoder";
    AMediaCodec* dec = AMediaCodec_createCodecByName(dec_name);
    if (!dec) { LOG("FAIL: create decoder %s", dec_name); return 1; }

    auto nals = nal_offsets(kStream, kStreamLen);
    LOG("stream %zu bytes, %zu NALs", (size_t)kStreamLen, nals.size());
    if (nals.size() < 4) { LOG("FAIL: bad embedded stream"); return 4; }

    // 提取 SPS/PPS 作 csd-0/csd-1（C2 软解 configure 需要参数集，否则 BAD_VALUE）
    const uint8_t* csd0 = nullptr; size_t csd0_len = 0;
    const uint8_t* csd1 = nullptr; size_t csd1_len = 0;
    for (size_t i = 0; i + 1 < nals.size(); i++) {
        size_t s = nals[i];
        size_t e = (i + 1 < nals.size()) ? nals[i + 1] : kStreamLen;
        const uint8_t* p = kStream + s;
        size_t skip = (p[2] == 1) ? 3 : 4;  // 起始码长
        uint8_t ntype = (e - s > skip) ? (p[skip] & 0x1f) : 0;
        if (ntype == 7 && !csd0) { csd0 = p; csd0_len = e - s; }
        if (ntype == 8 && !csd1) { csd1 = p; csd1_len = e - s; }
    }

    AMediaFormat* fmt = AMediaFormat_new();
    AMediaFormat_setString(fmt, AMEDIAFORMAT_KEY_MIME, "video/avc");
    // CCodec 对视频组件强制要求 width/height（CCodec.cpp Enforce required parameters）；
    // 176x144 为内置流 SPS 实际尺寸（提示值，真实输出尺寸仍由解码决定）
    AMediaFormat_setInt32(fmt, AMEDIAFORMAT_KEY_WIDTH, 176);
    AMediaFormat_setInt32(fmt, AMEDIAFORMAT_KEY_HEIGHT, 144);
    if (csd0) AMediaFormat_setBuffer(fmt, "csd-0", csd0, csd0_len);
    if (csd1) AMediaFormat_setBuffer(fmt, "csd-1", csd1, csd1_len);
    media_status_t st = AMediaCodec_configure(dec, fmt, nullptr, nullptr, 0);
    AMediaFormat_delete(fmt);
    if (st != AMEDIA_OK) { LOG("FAIL: configure decoder %d", st); return 2; }
    if (AMediaCodec_start(dec) != AMEDIA_OK) { LOG("FAIL: start decoder"); return 3; }

    size_t fed_idx = 0;
    int got_frames = 0, rendered = 0, eos_queued = 0;
    for (int iter = 0; iter < 4000; iter++) {
        // 喂入：一次一个 NAL（含起始码）
        if (fed_idx < nals.size()) {
            ssize_t ib = AMediaCodec_dequeueInputBuffer(dec, 100000 /*100ms*/);
            if (ib >= 0) {
                size_t cap = 0; uint8_t* buf = AMediaCodec_getInputBuffer(dec, ib, &cap);
                size_t start = nals[fed_idx];
                size_t end = (fed_idx + 1 < nals.size()) ? nals[fed_idx + 1] : kStreamLen;
                size_t sz = end - start;
                if (buf && sz <= cap) {
                    memcpy(buf, kStream + start, sz);
                    bool last = (fed_idx + 1 == nals.size());
                    AMediaCodec_queueInputBuffer(dec, ib, 0, sz,
                                                 (uint64_t)fed_idx * 33333,
                                                 last ? AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM : 0);
                    if (last) eos_queued = 1;
                    fed_idx++;
                } else if (!buf) {
                    LOG("input buffer null (ib=%zd)", ib);
                }
            }
        }
        // 取出
        AMediaCodecBufferInfo info;
        ssize_t ob = AMediaCodec_dequeueOutputBuffer(dec, &info, 100000);
        if (ob >= 0) {
            if (info.size > 0) { got_frames++; rendered++; }
            AMediaCodec_releaseOutputBuffer(dec, ob, info.size > 0);
        } else if (ob == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            AMediaFormat* of = AMediaCodec_getOutputFormat(dec);
            LOG("output format: %s", AMediaFormat_toString(of));
            AMediaFormat_delete(of);
        }
        // EOS 输出后收工
        if (eos_queued && ob >= 0 && (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM)) break;
        if (eos_queued && fed_idx == nals.size() && ob == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            // 容错：个别实现不回传 EOS 标志，再等若干轮后退出
            static int dry = 0;
            if (++dry > 30) break;
        }
    }

    AMediaCodec_stop(dec);
    AMediaCodec_delete(dec);
    LOG("decoded frames: %d (rendered %d), fed %zu/%zu NALs",
        got_frames, rendered, fed_idx, nals.size());
    if (got_frames < 30) { LOG("FAIL: decoded %d < 30 frames", got_frames); return 5; }
    LOG("PASS: media NDK (AMediaCodec AVC decode of embedded stream)");
    return 0;
}
