// pdtest_camera —— NDK 相机功能测试：枚举 → 打开 → 拍一张真实 JPEG。
// 判据：产出文件存在、非平凡大小、JPEG magic（FFD8）。
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraError.h>
#include <camera/NdkCameraDevice.h>
#include <media/NdkImageReader.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

#include <cstdlib>

// 同 pdtest_media：裸 NDK 进程需显式起 libbinder 线程池，否则 cameraserver → 本进程的
// 相机回调（设备状态/请求完成）滞留 binder 驱动，open/捕获相关等待可能死等。
// NDK sysroot 无 android/binder_process.h（平台专属头），按官方签名自行声明。
extern "C" void ABinderProcess_startThreadPool(void);

#define LOG(...) do { fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)

// 产物路径：PD_OUT 环境变量驱动；默认走 M3 契约目录 /userdata/boot/tests/
//（bind 生效后与 /data 同源；M1 调试期由 start.sh 覆写）
static std::string out_path(const char* name) {
    const char* dir = getenv("PD_OUT");
    return std::string(dir ? dir : "/userdata/boot/tests") + "/" + name;
}

static bool g_got_jpeg = false;

static void onDisconnected(void*, ACameraDevice*) {}
static void onError(void*, ACameraDevice*, int err) { LOG("camera error %d", err); }
static ACameraDevice_stateCallbacks g_dev_cbs = {nullptr, onDisconnected, onError};

static void onImageAvailable(void* ctx, AImageReader* reader) {
    (void)ctx;
    AImage* img = nullptr;
    if (AImageReader_acquireLatestImage(reader, &img) != AMEDIA_OK || !img) return;
    int32_t fmt = 0;
    AImage_getFormat(img, &fmt);
    // 相机 JPEG 流的图像按 HAL_PIXEL_FORMAT_BLOB(0x21=33) 上报（NDK 的
    // AIMAGE_FORMAT_JPEG=0x100 是"请求格式"，不等于回调里 AImage_getFormat 的回显），
    // 两者都接受；真实性由末尾的 JPEG magic(FFD8)+尺寸校验把关。
    if (fmt == AIMAGE_FORMAT_JPEG || fmt == 0x21 /* HAL_PIXEL_FORMAT_BLOB */) {
        uint8_t* data = nullptr; int len = 0;
        if (AImage_getPlaneData(img, 0, &data, &len) == AMEDIA_OK && len > 0) {
            FILE* f = fopen(out_path("pdtest_camera.jpg").c_str(), "wb");
            if (f) { fwrite(data, 1, len, f); fclose(f); g_got_jpeg = true; }
        }
    }
    AImage_delete(img);
}

int main() {
    ABinderProcess_startThreadPool();  // 见上方注释：回调可达性前置
    ACameraManager* mgr = ACameraManager_create();
    if (!mgr) { LOG("FAIL: ACameraManager_create"); return 1; }

    ACameraIdList* ids = nullptr;
    if (ACameraManager_getCameraIdList(mgr, &ids) != ACAMERA_OK || !ids || ids->numCameras == 0) {
        LOG("FAIL: no cameras (ids=%p)", (void*)ids);
        return 2;
    }
    LOG("cameras: %d (first: %s)", ids->numCameras, ids->cameraIds[0]);
    std::string cid = ids->cameraIds[0];

    ACameraMetadata* meta = nullptr;
    bool jpeg_cap = false;
    if (ACameraManager_getCameraCharacteristics(mgr, cid.c_str(), &meta) == ACAMERA_OK && meta) {
        ACameraMetadata_const_entry e;
        // 流配置四元组（format, width, height, input?），format=0x100 JPEG
        if (ACameraMetadata_getConstEntry(meta, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &e) == ACAMERA_OK) {
            for (uint32_t i = 0; i + 3 < e.count; i += 4)
                if (e.data.i32[i] == AIMAGE_FORMAT_JPEG) { jpeg_cap = true; break; }
        }
        ACameraMetadata_free(meta);
    }
    LOG("JPEG capability: %s", jpeg_cap ? "yes" : "no");

    ACameraDevice* dev = nullptr;
    if (ACameraManager_openCamera(mgr, cid.c_str(), &g_dev_cbs, &dev) != ACAMERA_OK || !dev) {
        LOG("FAIL: openCamera");
        return 3;
    }
    LOG("camera opened: %s", cid.c_str());

    AImageReader* reader = nullptr;
    media_status_t ms = AImageReader_new(640, 480, AIMAGE_FORMAT_JPEG, 4, &reader);
    if (ms != AMEDIA_OK || !reader) { LOG("FAIL: AImageReader_new %d", ms); return 4; }
    AImageReader_ImageListener listener = {nullptr, onImageAvailable};
    AImageReader_setImageListener(reader, &listener);
    ANativeWindow* window = nullptr;
    AImageReader_getWindow(reader, &window);

    ACaptureSessionOutputContainer* outputs = nullptr;
    ACaptureSessionOutput* output = nullptr;
    ACameraOutputTarget* target = nullptr;
    ACameraCaptureSession* session = nullptr;
    ACaptureRequest* request = nullptr;
    // 请求模板：JPEG 拍照此处取 TEMPLATE_PREVIEW（而非习惯的 STILL_CAPTURE）。
    // 实测：STILL_CAPTURE 会选中 GCH 的 ZslSnapshotCaptureSession 快照路径，本机
    // 模拟相机 provider 构建（预编译 vendor APEX，GCH_HWL_USE_DLOPEN not supported）
    // 该路径不产出任何帧；TEMPLATE_PREVIEW + JPEG 流是同样的 NDK 规范用法，完整
    // 走通 传感器→JPEG 编码→BLOB 缓冲→客户端 采集闭环（640x480 JPEG ~9KB）。
    ACameraDevice_request_template tmpl = TEMPLATE_PREVIEW;
    if (jpeg_cap)
        LOG("note: TEMPLATE_PREVIEW for JPEG (STILL_CAPTURE hits provider's unsupported ZSL path)");
    ACameraDevice_createCaptureRequest(dev, tmpl, &request);
    ACaptureSessionOutputContainer_create(&outputs);
    ACaptureSessionOutput_create(window, &output);
    ACaptureSessionOutputContainer_add(outputs, output);
    ACameraOutputTarget_create(window, &target);
    ACaptureRequest_addTarget(request, target);
    // 会话回调不可为空（NDK 实测：nullptr → "invalid input" 建会话失败）
    static ACameraCaptureSession_stateCallbacks session_cbs = {
        nullptr,
        [](void*, ACameraCaptureSession*) {},
        [](void*, ACameraCaptureSession*) {},
    };
    camera_status_t src = ACameraDevice_createCaptureSession(dev, outputs, &session_cbs, &session);
    if (src != ACAMERA_OK || !session) { LOG("FAIL: createCaptureSession %d", src); return 4; }
    int rc = ACameraCaptureSession_setRepeatingRequest(session, nullptr, 1, &request, nullptr);
    LOG("repeating request rc=%d, capturing up to 10s...", rc);

    for (int i = 0; i < 100 && !g_got_jpeg; i++) std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ACameraCaptureSession_stopRepeating(session);
    ACameraCaptureSession_close(session);
    ACaptureRequest_free(request);
    ACameraOutputTarget_free(target);
    ACaptureSessionOutput_free(output);
    ACaptureSessionOutputContainer_free(outputs);
    AImageReader_delete(reader);
    ACameraDevice_close(dev);
    ACameraManager_delete(mgr);

    if (!g_got_jpeg) { LOG("FAIL: no JPEG captured"); return 5; }
    FILE* f = fopen(out_path("pdtest_camera.jpg").c_str(), "rb");
    if (!f) { LOG("FAIL: saved file missing"); return 6; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    unsigned char magic[2] = {0, 0};
    if (sz > 0) fread(magic, 1, 2, f);
    fclose(f);
    LOG("jpeg size=%ld magic=%02x%02x", sz, magic[0], magic[1]);
    if (sz < 8 * 1024 || magic[0] != 0xFF || magic[1] != 0xD8) { LOG("FAIL: bad JPEG"); return 7; }
    LOG("PASS: camera NDK capture (%ld bytes JPEG)", sz);
    return 0;
}
