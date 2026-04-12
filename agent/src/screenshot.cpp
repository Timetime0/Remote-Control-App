#include "screenshot.h"

#include <string>
#include <vector>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <gdiplus.h>
#include <objidl.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;
#endif

static std::string escapeJson(const std::string& input) {
    std::string out;
    for (char c : input) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': break;
        default: out += c;
        }
    }
    return out;
}

#ifdef _WIN32

static int getEncoderClsid(const WCHAR* mimeType, CLSID* clsid) {
    UINT num = 0;
    UINT size = 0;

    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    std::vector<BYTE> buffer(size);
    ImageCodecInfo* codecs = reinterpret_cast<ImageCodecInfo*>(buffer.data());

    GetImageEncoders(num, size, codecs);

    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(codecs[i].MimeType, mimeType) == 0) {
            *clsid = codecs[i].Clsid;
            return static_cast<int>(i);
        }
    }

    return -1;
}

static std::string base64Encode(const BYTE* data, size_t len) {
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string out;

    for (size_t i = 0; i < len; i += 3) {
        unsigned int value = 0;
        int count = 0;

        for (int j = 0; j < 3; ++j) {
            value <<= 8;
            if (i + j < len) {
                value |= data[i + j];
                ++count;
            }
        }

        for (int j = 0; j < 4; ++j) {
            if (j <= count) {
                out += table[(value >> (18 - j * 6)) & 0x3F];
            }
            else {
                out += '=';
            }
        }
    }

    return out;
}

std::string captureScreenshotJson() {
    GdiplusStartupInput gdiplusStartupInput{};
    ULONG_PTR gdiplusToken = 0;

    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) != Ok) {
        return "{\"ok\":false,\"command\":\"SCREENSHOT\",\"message\":\"gdiplus_start_failed\"}";
    }

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HDC screenDc = GetDC(nullptr);
    HDC memoryDc = CreateCompatibleDC(screenDc);
    HBITMAP hBitmap = CreateCompatibleBitmap(screenDc, width, height);

    if (!screenDc || !memoryDc || !hBitmap) {
        if (hBitmap) DeleteObject(hBitmap);
        if (memoryDc) DeleteDC(memoryDc);
        if (screenDc) ReleaseDC(nullptr, screenDc);
        GdiplusShutdown(gdiplusToken);

        return "{\"ok\":false,\"command\":\"SCREENSHOT\",\"message\":\"capture_init_failed\"}";
    }

    HGDIOBJ oldObj = SelectObject(memoryDc, hBitmap);

    if (!BitBlt(memoryDc, 0, 0, width, height, screenDc, 0, 0, SRCCOPY | CAPTUREBLT)) {
        SelectObject(memoryDc, oldObj);
        DeleteObject(hBitmap);
        DeleteDC(memoryDc);
        ReleaseDC(nullptr, screenDc);
        GdiplusShutdown(gdiplusToken);

        return "{\"ok\":false,\"command\":\"SCREENSHOT\",\"message\":\"bitblt_failed\"}";
    }

    Bitmap* bitmap = Bitmap::FromHBITMAP(hBitmap, nullptr);
    if (!bitmap) {
        SelectObject(memoryDc, oldObj);
        DeleteObject(hBitmap);
        DeleteDC(memoryDc);
        ReleaseDC(nullptr, screenDc);
        GdiplusShutdown(gdiplusToken);

        return "{\"ok\":false,\"command\":\"SCREENSHOT\",\"message\":\"bitmap_failed\"}";
    }

    CLSID pngClsid{};
    if (getEncoderClsid(L"image/png", &pngClsid) < 0) {
        delete bitmap;
        SelectObject(memoryDc, oldObj);
        DeleteObject(hBitmap);
        DeleteDC(memoryDc);
        ReleaseDC(nullptr, screenDc);
        GdiplusShutdown(gdiplusToken);

        return "{\"ok\":false,\"command\":\"SCREENSHOT\",\"message\":\"png_encoder_not_found\"}";
    }

    IStream* stream = nullptr;
    if (CreateStreamOnHGlobal(nullptr, TRUE, &stream) != S_OK) {
        delete bitmap;
        SelectObject(memoryDc, oldObj);
        DeleteObject(hBitmap);
        DeleteDC(memoryDc);
        ReleaseDC(nullptr, screenDc);
        GdiplusShutdown(gdiplusToken);

        return "{\"ok\":false,\"command\":\"SCREENSHOT\",\"message\":\"stream_failed\"}";
    }

    Status saveStatus = bitmap->Save(stream, &pngClsid, nullptr);
    if (saveStatus != Ok) {
        stream->Release();
        delete bitmap;
        SelectObject(memoryDc, oldObj);
        DeleteObject(hBitmap);
        DeleteDC(memoryDc);
        ReleaseDC(nullptr, screenDc);
        GdiplusShutdown(gdiplusToken);

        return "{\"ok\":false,\"command\":\"SCREENSHOT\",\"message\":\"save_failed\"}";
    }

    HGLOBAL hGlobal = nullptr;
    if (GetHGlobalFromStream(stream, &hGlobal) != S_OK) {
        stream->Release();
        delete bitmap;
        SelectObject(memoryDc, oldObj);
        DeleteObject(hBitmap);
        DeleteDC(memoryDc);
        ReleaseDC(nullptr, screenDc);
        GdiplusShutdown(gdiplusToken);

        return "{\"ok\":false,\"command\":\"SCREENSHOT\",\"message\":\"stream_data_failed\"}";
    }

    SIZE_T size = GlobalSize(hGlobal);
    BYTE* bytes = static_cast<BYTE*>(GlobalLock(hGlobal));

    std::string base64;
    if (bytes && size > 0) {
        base64 = base64Encode(bytes, static_cast<size_t>(size));
    }

    if (bytes) {
        GlobalUnlock(hGlobal);
    }

    stream->Release();
    delete bitmap;

    SelectObject(memoryDc, oldObj);
    DeleteObject(hBitmap);
    DeleteDC(memoryDc);
    ReleaseDC(nullptr, screenDc);

    GdiplusShutdown(gdiplusToken);

    return "{"
        "\"ok\":true,"
        "\"command\":\"SCREENSHOT\","
        "\"mime\":\"image/png\","
        "\"data\":\"" + escapeJson(base64) + "\""
        "}";
}

#else

std::string captureScreenshotJson() {
    return "{\"ok\":false,\"command\":\"SCREENSHOT\",\"message\":\"not_supported\"}";
}

#endif