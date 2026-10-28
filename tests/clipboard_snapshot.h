#pragma once
#include <windows.h>
#include <ole2.h>
#include <shlobj.h>
#include <wrl/client.h>
#include <string>
#include <thread>
#include <vector>
#include <utility>

// OleGetClipboard returns a live proxy, not a snapshot. Setting that proxy
// back as the clipboard owner makes GetData recurse into itself: when a
// running OLE application (a browser, terminal or editor) owned the
// clipboard, OleFlushClipboard overflowed the stack inside the clipboard
// window procedure and Windows ended the test with 0xc000041d.
// Materialize independent handles before exercising the clipboard path instead.
struct ClipboardSnapshot {
    HWND owner;
    bool captured = false;
    std::vector<std::pair<UINT,HANDLE>> data;
    // Clipboard history and other listeners briefly open it after each change.
    static bool open(HWND hwnd) {
        for(int attempt=0;attempt<100;++attempt){if(OpenClipboard(hwnd))return true;Sleep(10);}
        return false;
    }
    explicit ClipboardSnapshot(HWND hwnd) : owner(hwnd) {
        if(!open(hwnd))return;
        for(UINT format=EnumClipboardFormats(0);format;format=EnumClipboardFormats(format)) {
            if(format==CF_OWNERDISPLAY)continue;
            HANDLE value=GetClipboardData(format);
            if(value)if(HANDLE copy=OleDuplicateData(value,static_cast<CLIPFORMAT>(format),0))data.push_back({format,copy});
        }
        CloseClipboard();
        captured = true;
    }
    ~ClipboardSnapshot() {
        // Do not overwrite anything the user copied while tests were running.
        if(captured && GetClipboardOwner()==owner && open(owner)) {
            EmptyClipboard();
            for(auto& item:data)if(SetClipboardData(item.first,item.second))item.second=nullptr;
            CloseClipboard();
        }
        for(const auto& item:data)if(item.second) {
            if(item.first==CF_BITMAP||item.first==CF_DSPBITMAP||item.first==CF_PALETTE)DeleteObject(item.second);
            else if(item.first==CF_ENHMETAFILE||item.first==CF_DSPENHMETAFILE)DeleteEnhMetaFile((HENHMETAFILE)item.second);
            else {
                if(item.first==CF_METAFILEPICT||item.first==CF_DSPMETAFILEPICT) {
                    auto picture=(METAFILEPICT*)GlobalLock(item.second);if(picture){DeleteMetaFile(picture->hMF);GlobalUnlock(item.second);}
                }
                GlobalFree(item.second);
            }
        }
    }
};

// Puts text on the clipboard the way a running OLE application does: the data
// object stays live on its own STA thread and renders each format on demand.
// Tests snapshot this state to cover clipboards copied from other programs.
struct OleClipboardSource {
    HANDLE ready = CreateEventW(nullptr,TRUE,FALSE,nullptr);
    HANDLE quit = CreateEventW(nullptr,TRUE,FALSE,nullptr);
    bool owned = false;
    std::thread thread;
    explicit OleClipboardSource(std::wstring text) : thread([this,text]{run(text);}) { wait(ready); }
    ~OleClipboardSource() {
        SetEvent(quit);wait(thread.native_handle());thread.join();
        CloseHandle(ready);CloseHandle(quit);
    }
    // Clipboard ownership changes notify the previous owner, which may be a
    // test window on this thread, so keep serving sent messages while waiting.
    static void wait(HANDLE handle) {
        while(MsgWaitForMultipleObjects(1,&handle,FALSE,5000,QS_SENDMESSAGE)==WAIT_OBJECT_0+1) {
            MSG message;PeekMessageW(&message,nullptr,0,0,PM_NOREMOVE|PM_QS_SENDMESSAGE);
        }
    }
    void run(const std::wstring& text) {
        if(FAILED(OleInitialize(nullptr))){SetEvent(ready);return;}
        Microsoft::WRL::ComPtr<IDataObject> source;
        size_t bytes=(text.size()+1)*sizeof(wchar_t);
        HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,bytes);
        if(memory)if(void* target=GlobalLock(memory)){memcpy(target,text.c_str(),bytes);GlobalUnlock(memory);}
        FORMATETC format{CF_UNICODETEXT,nullptr,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
        STGMEDIUM medium{};medium.tymed=TYMED_HGLOBAL;medium.hGlobal=memory;
        if(memory && SUCCEEDED(SHCreateDataObject(nullptr,0,nullptr,nullptr,IID_PPV_ARGS(&source))) &&
           SUCCEEDED(source->SetData(&format,&medium,TRUE))) {
            for(int attempt=0;attempt<100 && !owned;++attempt)
                if(!(owned=SUCCEEDED(OleSetClipboard(source.Get()))))Sleep(10);
        } else if(memory)GlobalFree(memory);
        SetEvent(ready);
        // Serve render requests until the test is done with the clipboard.
        while(MsgWaitForMultipleObjects(1,&quit,FALSE,INFINITE,QS_ALLINPUT)==WAIT_OBJECT_0+1) {
            MSG message;while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
        }
        source.Reset();
        OleUninitialize();
    }
};

// Holds the clipboard open on its own thread, as Remote Desktop's clipboard
// sync and clipboard history do for a moment after each change.
struct ClipboardHolder {
    HANDLE ready = CreateEventW(nullptr,TRUE,FALSE,nullptr);
    bool opened = false;
    std::thread thread;
    explicit ClipboardHolder(DWORD milliseconds) : thread([this,milliseconds]{
        opened = ClipboardSnapshot::open(nullptr);
        SetEvent(ready);
        if(opened){Sleep(milliseconds);CloseClipboard();}
    }) { WaitForSingleObject(ready,INFINITE); }
    ~ClipboardHolder(){thread.join();CloseHandle(ready);}
};
