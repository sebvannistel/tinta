#include "file_icon.h"
#include "settings.h"

#include <windows.h>
#include <appmodel.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <wincodec.h>

#include <cwctype>
#include <fstream>
#include <iterator>

namespace {

std::wstring regString(HKEY root, const std::wstring& key, const wchar_t* value) {
    wchar_t buf[1024] = {};
    DWORD size = sizeof(buf) - sizeof(wchar_t);
    if (RegGetValueW(root, key.c_str(), value, RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ |
                     RRF_NOEXPAND, nullptr, buf, &size) != ERROR_SUCCESS)
        return L"";
    return buf;
}

// "@{...}" and "@file,-id" strings as Explorer shows them
std::wstring indirect(const std::wstring& s) {
    if (s.empty() || s[0] != L'@') return s;
    wchar_t out[1024] = {};
    if (SUCCEEDED(SHLoadIndirectString(s.c_str(), out, 1024, nullptr))) return out;
    return L"";
}

bool fileExists(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

std::wstring lowerExtension(const std::wstring& path) {
    std::wstring ext = PathFindExtensionW(path.c_str());
    for (wchar_t& c : ext) c = (wchar_t)towlower(c);
    return ext;
}

std::string readBytes(const std::wstring& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), {});
}

bool writeBytes(const std::wstring& path, const std::string& bytes) {
    if (bytes.empty()) return false;
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(bytes.data(), (std::streamsize)bytes.size());
    return (bool)out;
}

// A 256px rendering of an exe/dll/ico icon, encoded as PNG
std::string iconResourceAsPng(const std::wstring& path, int index) {
    HICON icon = nullptr;
    if (FAILED(SHDefExtractIconW(path.c_str(), index, 0, &icon, nullptr, 256)) || !icon)
        return "";
    std::string png;
    HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    IWICImagingFactory* factory = nullptr;
    IWICBitmap* bitmap = nullptr;
    IStream* stream = nullptr;
    IWICBitmapEncoder* encoder = nullptr;
    IWICBitmapFrameEncode* frame = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_PPV_ARGS(&factory))) &&
        SUCCEEDED(factory->CreateBitmapFromHICON(icon, &bitmap)) &&
        SUCCEEDED(CreateStreamOnHGlobal(nullptr, TRUE, &stream)) &&
        SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) &&
        SUCCEEDED(encoder->Initialize(stream, WICBitmapEncoderNoCache)) &&
        SUCCEEDED(encoder->CreateNewFrame(&frame, nullptr)) &&
        SUCCEEDED(frame->Initialize(nullptr)) &&
        SUCCEEDED(frame->WriteSource(bitmap, nullptr)) &&
        SUCCEEDED(frame->Commit()) && SUCCEEDED(encoder->Commit())) {
        HGLOBAL mem = nullptr;
        if (SUCCEEDED(GetHGlobalFromStream(stream, &mem))) {
            STATSTG stat = {};
            stream->Stat(&stat, STATFLAG_NONAME);
            const char* data = (const char*)GlobalLock(mem);
            if (data) png.assign(data, (size_t)stat.cbSize.QuadPart);
            GlobalUnlock(mem);
        }
    }
    if (frame) frame->Release();
    if (encoder) encoder->Release();
    if (stream) stream->Release();
    if (bitmap) bitmap->Release();
    if (factory) factory->Release();
    if (SUCCEEDED(init)) CoUninitialize();
    DestroyIcon(icon);
    return png;
}

// The PNG resized to each of sizes (high-quality cubic), each as a PNG.
// Explorer picks the exact size for its views instead of shrinking the
// 256px image itself, which looked soft at 16px.
std::vector<std::string> resizedPngs(const std::string& png, const std::vector<UINT>& sizes) {
    std::vector<std::string> out;
    HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    IWICImagingFactory* factory = nullptr;
    IStream* in = SHCreateMemStream((const BYTE*)png.data(), (UINT)png.size());
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* source = nullptr;
    IWICFormatConverter* rgba = nullptr;
    if (in &&
        SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_PPV_ARGS(&factory))) &&
        SUCCEEDED(factory->CreateDecoderFromStream(in, nullptr, WICDecodeMetadataCacheOnLoad,
                                                   &decoder)) &&
        SUCCEEDED(decoder->GetFrame(0, &source)) &&
        SUCCEEDED(factory->CreateFormatConverter(&rgba)) &&
        SUCCEEDED(rgba->Initialize(source, GUID_WICPixelFormat32bppPBGRA,
                                   WICBitmapDitherTypeNone, nullptr, 0,
                                   WICBitmapPaletteTypeCustom))) {
        for (UINT size : sizes) {
            IWICBitmapScaler* scaler = nullptr;
            IStream* stream = nullptr;
            IWICBitmapEncoder* encoder = nullptr;
            IWICBitmapFrameEncode* frame = nullptr;
            if (SUCCEEDED(factory->CreateBitmapScaler(&scaler)) &&
                SUCCEEDED(scaler->Initialize(rgba, size, size,
                                             WICBitmapInterpolationModeHighQualityCubic)) &&
                SUCCEEDED(CreateStreamOnHGlobal(nullptr, TRUE, &stream)) &&
                SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) &&
                SUCCEEDED(encoder->Initialize(stream, WICBitmapEncoderNoCache)) &&
                SUCCEEDED(encoder->CreateNewFrame(&frame, nullptr)) &&
                SUCCEEDED(frame->Initialize(nullptr)) &&
                SUCCEEDED(frame->WriteSource(scaler, nullptr)) &&
                SUCCEEDED(frame->Commit()) && SUCCEEDED(encoder->Commit())) {
                HGLOBAL mem = nullptr;
                STATSTG stat = {};
                if (SUCCEEDED(GetHGlobalFromStream(stream, &mem)) &&
                    SUCCEEDED(stream->Stat(&stat, STATFLAG_NONAME))) {
                    const char* data = (const char*)GlobalLock(mem);
                    if (data) out.emplace_back(data, (size_t)stat.cbSize.QuadPart);
                    GlobalUnlock(mem);
                }
            }
            if (frame) frame->Release();
            if (encoder) encoder->Release();
            if (stream) stream->Release();
            if (scaler) scaler->Release();
        }
    }
    if (rgba) rgba->Release();
    if (source) source->Release();
    if (decoder) decoder->Release();
    if (in) in->Release();
    if (factory) factory->Release();
    if (SUCCEEDED(init)) CoUninitialize();
    if (out.size() != sizes.size()) out.clear();
    return out;
}

// Every size Explorer draws a file icon at, up to 100%-200% display scaling
std::string pngAsMultiSizeIco(const std::string& png) {
    return encodeIco(resizedPngs(png, {16, 20, 24, 32, 40, 48, 64, 96, 128, 256}));
}

}  // namespace

std::vector<BorrowableFileIcon> findBorrowableFileIcons() {
    std::vector<BorrowableFileIcon> icons;
    HKEY key;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L".md\\OpenWithProgids", 0, KEY_READ, &key) !=
        ERROR_SUCCESS)
        return icons;
    wchar_t progId[256];
    for (DWORD i = 0;; i++) {
        DWORD len = 256;
        if (RegEnumValueW(key, i, progId, &len, nullptr, nullptr, nullptr, nullptr) !=
            ERROR_SUCCESS)
            break;
        std::wstring id = progId;
        if (id == L"Tinta.MarkdownFile") continue;
        std::wstring source = regString(HKEY_CLASSES_ROOT, id + L"\\DefaultIcon", nullptr);
        std::wstring path;
        int index = 0;
        if (source.empty() || !resolveIconSource(source, path, index)) continue;
        std::wstring name =
            indirect(regString(HKEY_CLASSES_ROOT, id + L"\\Application", L"ApplicationName"));
        if (name.empty()) name = indirect(regString(HKEY_CLASSES_ROOT, id, nullptr));
        if (name.empty()) name = id;
        icons.push_back({name, source});
    }
    RegCloseKey(key);
    return icons;
}

std::wstring borrowedFileIconPath() {
    std::wstring dir = tintaConfigDir();
    return dir.empty() ? L"" : dir + L"\\file-icon.ico";
}

bool hasBorrowedFileIcon() {
    std::wstring path = borrowedFileIconPath();
    return !path.empty() && fileExists(path);
}

bool resolveIconSource(const std::wstring& iconSource, std::wstring& path, int& index) {
    index = 0;
    if (iconSource.rfind(L"@{", 0) == 0) {
        // Store app: @{PackageFullName?ms-resource://Name/Files/Assets/x.png}
        path = indirect(iconSource);
        if (fileExists(path)) return true;
        size_t q = iconSource.find(L'?');
        size_t files = iconSource.find(L"/Files/");
        if (q == std::wstring::npos || files == std::wstring::npos) return false;
        std::wstring package = iconSource.substr(2, q - 2);
        std::wstring rel = iconSource.substr(files + 7);
        if (!rel.empty() && rel.back() == L'}') rel.pop_back();
        for (wchar_t& c : rel) if (c == L'/') c = L'\\';
        UINT32 len = MAX_PATH;
        wchar_t root[MAX_PATH];
        if (GetPackagePathByFullName(package.c_str(), &len, root) != ERROR_SUCCESS)
            return false;
        path = std::wstring(root) + L"\\" + rel;
        return fileExists(path);
    }

    std::wstring s = iconSource;
    size_t comma = s.find_last_of(L',');
    if (comma != std::wstring::npos) {
        std::wstring tail = s.substr(comma + 1);
        wchar_t* end = nullptr;
        long value = wcstol(tail.c_str(), &end, 10);
        if (!tail.empty() && end && *end == 0) {
            index = (int)value;
            s.erase(comma);
        }
    }
    while (!s.empty() && iswspace(s.back())) s.pop_back();
    if (s.size() >= 2 && s.front() == L'"' && s.back() == L'"') s = s.substr(1, s.size() - 2);
    wchar_t expanded[MAX_PATH];
    if (ExpandEnvironmentStringsW(s.c_str(), expanded, MAX_PATH) - 1 < MAX_PATH) s = expanded;
    path = s;
    return !path.empty();
}

std::string encodeIco(const std::vector<std::string>& pngs) {
    static const char sig[] = "\x89PNG\r\n\x1a\n";
    if (pngs.empty()) return "";
    std::string ico, images;
    auto put16 = [&](uint16_t v) { ico += (char)(v & 0xff); ico += (char)(v >> 8); };
    auto put32 = [&](uint32_t v) { put16((uint16_t)(v & 0xffff)); put16((uint16_t)(v >> 16)); };
    put16(0); put16(1); put16((uint16_t)pngs.size());  // ICONDIR: reserved, icon, count
    uint32_t offset = 6 + 16 * (uint32_t)pngs.size();    // images follow the entries
    for (const std::string& png : pngs) {
        if (png.size() < 24 || png.compare(0, 8, sig, 8) != 0) return "";
        auto be32 = [&](size_t at) {
            return ((uint32_t)(uint8_t)png[at] << 24) | ((uint32_t)(uint8_t)png[at + 1] << 16) |
                   ((uint32_t)(uint8_t)png[at + 2] << 8) | (uint32_t)(uint8_t)png[at + 3];
        };
        uint32_t width = be32(16), height = be32(20);
        if (width == 0 || height == 0 || width > 256 || height > 256) return "";
        ico += (char)(width == 256 ? 0 : width);   // ICONDIRENTRY: 0 means 256
        ico += (char)(height == 256 ? 0 : height);
        ico += '\0'; ico += '\0';                  // no palette, reserved
        put16(1); put16(32);                       // planes, bits per pixel
        put32((uint32_t)png.size());
        put32(offset + (uint32_t)images.size());
        images += png;
    }
    return ico + images;
}

std::string wrapPngAsIco(const std::string& png) {
    return encodeIco({png});
}

bool copyFileIcon(const std::wstring& iconSource, const std::wstring& destIco) {
    std::wstring path;
    int index = 0;
    if (destIco.empty() || !resolveIconSource(iconSource, path, index) || !fileExists(path))
        return false;
    std::wstring ext = lowerExtension(path);
    if (ext == L".png") return writeBytes(destIco, pngAsMultiSizeIco(readBytes(path)));
    if (ext == L".ico" && index == 0) return CopyFileW(path.c_str(), destIco.c_str(), FALSE) != 0;
    return writeBytes(destIco, pngAsMultiSizeIco(iconResourceAsPng(path, index)));
}
