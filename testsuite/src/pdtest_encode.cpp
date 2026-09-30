// pdtest_encode —— AMediaCodec 编码回归测试：c2 软编 AVC 真实编码闭环。
// 防回归：历史上 CCodec 首次产出时 waitForService(package_native) 死等，
// dequeueOutputBuffer/getOutputFormat 永久不返回。本测试整体用 alarm(10) 兜底：
// 判据 = 流程在 10s 内走完且拿到 >=1 个输出 buffer（CODEC_CONFIG buffer 也算——
// 它是编码器最先吐出的东西）。
// 输入：3 帧合成 YUV420 平面数据；入队尺寸 = min(帧大小, input buffer cap)，
// 绝不越界写入（pdtest_media v1 教训：按 flexible 跨距整帧 memcpy 导致堆损坏；
// 这里不假设 stride，帧按 176*144*3/2 线性生成，实际写入以 getInputBuffer 的 cap 为准）。
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>

#include <unistd.h>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <chrono>

// 同 pdtest_media：裸 NDK 进程需显式起 libbinder 线程池，否则 C2 服务的 buffer 回调
// 永不送达（历史“首输出前永久挂死/package_native 死等”回归的根因之一）。
// NDK sysroot 无 android/binder_process.h（平台专属头），按官方签名自行声明。
extern "C" void ABinderProcess_startThreadPool(void);

#define LOG(...) do { fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)

static const int32_t kW = 176, kH = 144;
static const size_t kFrameBytes = (size_t)kW * kH * 3 / 2;  // YUV420 平面整帧

// 合成一帧：Y 平面梯度（按帧序偏移），U/V 近常量。不假设 stride，线性填充。
static void gen_frame(uint8_t* p, size_t n, int idx) {
    const size_t y_plane = (size_t)kW * kH;
    for (size_t i = 0; i < n; i++)
        p[i] = (i < y_plane) ? (uint8_t)((i / kW + i + idx * 40) & 0xff)
                             : (uint8_t)(128 + idx * 10);
}

// alarm 兜底：任一步死等（历史 package_native 回归）直接判 FAIL 退出，不挂死套件
static void on_alarm(int) {
    static const char msg[] =
        "FAIL: pdtest_encode timed out after 10s -- encoder pipeline hung "
        "(package_native waitForService regression?)\n";
    ssize_t rc = write(2, msg, sizeof(msg) - 1);
    (void)rc;
    _exit(9);
}

int main() {
    ABinderProcess_startThreadPool();  // 见上方注释：回调可达性前置
    signal(SIGALRM, on_alarm);
    alarm(10);

    AMediaCodec* enc = AMediaCodec_createEncoderByType("video/avc");
    if (!enc) enc = AMediaCodec_createCodecByName("c2.android.avc.encoder");
    if (!enc) { LOG("FAIL: create avc encoder"); return 1; }
    char* cname = nullptr;
    if (AMediaCodec_getName(enc, &cname) == AMEDIA_OK && cname) {
        LOG("encoder: %s", cname);
        AMediaCodec_releaseName(enc, cname);
    }

    // configure 输入必须有 WIDTH/HEIGHT（CCodec Enforce required parameters，见 pdtest_media 注释）；
    // 编码必须带 AMEDIACODEC_CONFIGURE_FLAG_ENCODE —— 缺省 0 时 MediaCodec 不在 format 里置
    // "encoder"，CCodec 的编码器名字校验（name 含 "encoder" vs encoder 标志）会以
    // UNKNOWN_ERROR 静默拒绝（实测 configure 返回 -10000）。
    // color-format 21 = 0x15 YUV420Planar，C2 软编标准接受格式。
    AMediaFormat* fmt = AMediaFormat_new();
    AMediaFormat_setString(fmt, AMEDIAFORMAT_KEY_MIME, "video/avc");
    AMediaFormat_setInt32(fmt, AMEDIAFORMAT_KEY_WIDTH, kW);
    AMediaFormat_setInt32(fmt, AMEDIAFORMAT_KEY_HEIGHT, kH);
    AMediaFormat_setInt32(fmt, AMEDIAFORMAT_KEY_BIT_RATE, 500000);
    AMediaFormat_setInt32(fmt, AMEDIAFORMAT_KEY_FRAME_RATE, 15);
    AMediaFormat_setInt32(fmt, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, 1);
    AMediaFormat_setInt32(fmt, AMEDIAFORMAT_KEY_COLOR_FORMAT, 21);
    media_status_t st = AMediaCodec_configure(enc, fmt, nullptr, nullptr,
                                              AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
    AMediaFormat_delete(fmt);
    if (st != AMEDIA_OK) { LOG("FAIL: configure encoder %d", st); AMediaCodec_delete(enc); return 2; }
    if (AMediaCodec_start(enc) != AMEDIA_OK) { LOG("FAIL: start encoder"); AMediaCodec_delete(enc); return 3; }

    int fed = 0, out_bufs = 0, fmt_changes = 0;
    size_t cfg_bytes = 0, es_bytes = 0;
    bool eos_queued = false, eos_out = false;
    int dry = 0;  // EOS 已喂入后的连续 TRY_AGAIN 计数（容错退出用）
    std::vector<uint8_t> frame(kFrameBytes);

    // 内部 8s 软期限先到先退；alarm(10) 只是最后防线，正常路径不应触发
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
    while (std::chrono::steady_clock::now() < deadline) {
        // 喂入：3 帧合成 YUV420，尺寸 = min(帧大小, cap)，不得越界入队
        if (fed < 3) {
            ssize_t ib = AMediaCodec_dequeueInputBuffer(enc, 100000 /*100ms*/);
            if (ib >= 0) {
                size_t cap = 0;
                uint8_t* buf = AMediaCodec_getInputBuffer(enc, ib, &cap);
                if (buf && cap > 0) {
                    gen_frame(frame.data(), kFrameBytes, fed);
                    size_t sz = kFrameBytes < cap ? kFrameBytes : cap;
                    memcpy(buf, frame.data(), sz);
                    bool last = (fed == 2);
                    AMediaCodec_queueInputBuffer(enc, ib, 0, sz,
                                                 (uint64_t)fed * 66667 /*15fps*/,
                                                 last ? AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM : 0);
                    if (last) eos_queued = true;
                    if (fed == 0) LOG("input cap=%zu, frame=%zu, queued=%zu", cap, kFrameBytes, sz);
                    fed++;
                } else {
                    LOG("input buffer null (ib=%zd)", ib);
                }
            }
        }

        // 取出：判据核心——dequeueOutputBuffer 必须在期限内返回且有产出
        AMediaCodecBufferInfo info;
        ssize_t ob = AMediaCodec_dequeueOutputBuffer(enc, &info, 100000);
        if (ob >= 0) {
            out_bufs++;
            bool is_cfg = (info.flags & AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG) != 0;
            if (info.size > 0) {
                if (is_cfg) cfg_bytes += (size_t)info.size;
                else es_bytes += (size_t)info.size;
            }
            LOG("output #%d: size=%d%s", out_bufs, info.size, is_cfg ? " (codec config)" : "");
            AMediaCodec_releaseOutputBuffer(enc, ob, false /*encoder 不渲染*/);
            if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) eos_out = true;
            dry = 0;
        } else if (ob == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            fmt_changes++;
            AMediaFormat* of = AMediaCodec_getOutputFormat(enc);
            LOG("output format: %s", of ? AMediaFormat_toString(of) : "(null)");
            if (of) AMediaFormat_delete(of);
            dry = 0;
        } else if (ob == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            // 容错：个别实现不回传 EOS 标志，EOS 喂完后再空转 30 轮即收工
            if (eos_queued && fed == 3 && ++dry > 30) break;
        }

        if (eos_out) break;
    }

    AMediaCodec_stop(enc);
    AMediaCodec_delete(enc);
    alarm(0);

    LOG("fed %d/3 frames, output buffers=%d (format-change x%d), es=%zu bytes, csd=%zu bytes",
        fed, out_bufs, fmt_changes, es_bytes, cfg_bytes);
    // 判据：10s 内拿到 >=1 个输出 buffer（codec config buffer 计入）
    if (out_bufs < 1) { LOG("FAIL: no output buffer within deadline (hung or no output)"); return 4; }
    LOG("PASS: encode NDK (AMediaCodec AVC encode, bitstream %zu bytes + csd %zu bytes)",
        es_bytes, cfg_bytes);
    return 0;
}
