#include <windows.h>
#include <streams.h>
#include <initguid.h>
#include <strsafe.h>

// {B1234567-0000-0000-0000-000000000001}
DEFINE_GUID(CLSID_VideoFilter,
    0xb1234567, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01);

#define FILTER_NAME     L"VIDEO File Source Filter"
#define PADDING_SIZE    64

// ============================================================
// CVideoStream — IStream wrapper שמדלג על ה-padding
// ============================================================
class CVideoStream : public IStream {
    HANDLE  m_hFile;
    LONG    m_refCount;
    LONGLONG m_fileSize;

public:
    CVideoStream(HANDLE hFile, LONGLONG fileSize)
        : m_hFile(hFile), m_refCount(1), m_fileSize(fileSize)
    {
        // דלג על ה-padding בפתיחה
        LARGE_INTEGER li;
        li.QuadPart = PADDING_SIZE;
        SetFilePointerEx(m_hFile, li, NULL, FILE_BEGIN);
    }

    ~CVideoStream() {
        if (m_hFile != INVALID_HANDLE_VALUE)
            CloseHandle(m_hFile);
    }

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) {
        if (riid == IID_IUnknown || riid == IID_IStream || riid == IID_ISequentialStream) {
            *ppv = static_cast<IStream*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef()  { return InterlockedIncrement(&m_refCount); }
    STDMETHODIMP_(ULONG) Release() {
        LONG ref = InterlockedDecrement(&m_refCount);
        if (ref == 0) delete this;
        return ref;
    }

    // ISequentialStream
    STDMETHODIMP Read(void* pv, ULONG cb, ULONG* pcbRead) {
        DWORD dwRead = 0;
        BOOL ok = ReadFile(m_hFile, pv, cb, &dwRead, NULL);
        if (pcbRead) *pcbRead = dwRead;
        return (ok || dwRead > 0) ? S_OK : S_FALSE;
    }
    STDMETHODIMP Write(const void*, ULONG, ULONG*) { return STG_E_ACCESSDENIED; }

    // IStream
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition) {
        LARGE_INTEGER newPos;
        if (dwOrigin == STREAM_SEEK_SET) {
            // offset מחושב מאחרי ה-padding
            dlibMove.QuadPart += PADDING_SIZE;
            SetFilePointerEx(m_hFile, dlibMove, &newPos, FILE_BEGIN);
            if (plibNewPosition)
                plibNewPosition->QuadPart = newPos.QuadPart - PADDING_SIZE;
        } else if (dwOrigin == STREAM_SEEK_CUR) {
            SetFilePointerEx(m_hFile, dlibMove, &newPos, FILE_CURRENT);
            if (plibNewPosition)
                plibNewPosition->QuadPart = newPos.QuadPart - PADDING_SIZE;
        } else { // STREAM_SEEK_END
            dlibMove.QuadPart -= 0; // END ביחס לסוף הקובץ
            SetFilePointerEx(m_hFile, dlibMove, &newPos, FILE_END);
            if (plibNewPosition)
                plibNewPosition->QuadPart = newPos.QuadPart - PADDING_SIZE;
        }
        return S_OK;
    }

    STDMETHODIMP Stat(STATSTG* pstatstg, DWORD) {
        ZeroMemory(pstatstg, sizeof(STATSTG));
        pstatstg->type = STGTY_STREAM;
        pstatstg->cbSize.QuadPart = m_fileSize - PADDING_SIZE;
        return S_OK;
    }

    STDMETHODIMP SetSize(ULARGE_INTEGER)                          { return E_NOTIMPL; }
    STDMETHODIMP CopyTo(IStream*, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER*) { return E_NOTIMPL; }
    STDMETHODIMP Commit(DWORD)                                    { return E_NOTIMPL; }
    STDMETHODIMP Revert()                                         { return E_NOTIMPL; }
    STDMETHODIMP LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) { return E_NOTIMPL; }
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) { return E_NOTIMPL; }
    STDMETHODIMP Clone(IStream**)                                 { return E_NOTIMPL; }
};

// ============================================================
// CVideoFilter — AsyncReader-style source filter
// ============================================================
class CVideoOutputPin;

class CVideoFilter : public CBaseFilter {
public:
    CVideoOutputPin* m_pPin;
    WCHAR            m_szFileName[MAX_PATH];
    LONGLONG         m_fileSize;

    CVideoFilter(LPUNKNOWN pUnk, HRESULT* phr);
    ~CVideoFilter();

    static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT* phr);

    int    GetPinCount()           override { return 1; }
    CBasePin* GetPin(int n)        override;

    HRESULT OpenFile(LPCWSTR pszFileName);
};

// ============================================================
// CVideoOutputPin
// ============================================================
class CVideoOutputPin : public CBasePin, public IAsyncReader {
    CVideoFilter* m_pFilter2;
    IStream*      m_pStream;
    LONGLONG      m_llLength;

public:
    CVideoOutputPin(CVideoFilter* pFilter, CCritSec* pLock, HRESULT* phr)
        : CBasePin(L"Output", pFilter, pLock, phr, L"Output", PINDIR_OUTPUT)
        , m_pFilter2(pFilter), m_pStream(NULL), m_llLength(0) {}

    ~CVideoOutputPin() { if (m_pStream) m_pStream->Release(); }

    HRESULT SetStream(IStream* pStream, LONGLONG length) {
        if (m_pStream) m_pStream->Release();
        m_pStream = pStream;
        m_pStream->AddRef();
        m_llLength = length;
        return S_OK;
    }

    // CBasePin
    HRESULT CheckMediaType(const CMediaType*) override { return S_OK; }
    HRESULT GetMediaType(int iPosition, CMediaType* pmt) override {
        if (iPosition != 0) return VFW_S_NO_MORE_ITEMS;
        pmt->SetType(&MEDIATYPE_Stream);
        pmt->SetSubtype(&MEDIASUBTYPE_NULL);
        return S_OK;
    }

    // IUnknown via CBasePin
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IAsyncReader) {
            *ppv = static_cast<IAsyncReader*>(this);
            AddRef();
            return S_OK;
        }
        return CBasePin::QueryInterface(riid, ppv);
    }

    // IAsyncReader
    STDMETHODIMP RequestAllocator(IMemAllocator*, ALLOCATOR_PROPERTIES*, IMemAllocator**) { return E_NOTIMPL; }
    STDMETHODIMP Request(IMediaSample*, DWORD_PTR)  { return E_NOTIMPL; }
    STDMETHODIMP WaitForNext(DWORD, IMediaSample**, DWORD_PTR*) { return E_NOTIMPL; }
    STDMETHODIMP BeginFlush()  { return S_OK; }
    STDMETHODIMP EndFlush()    { return S_OK; }

    STDMETHODIMP Length(LONGLONG* pTotal, LONGLONG* pAvailable) override {
        *pTotal = *pAvailable = m_llLength;
        return S_OK;
    }

    STDMETHODIMP SyncRead(LONGLONG llPosition, LONG lLength, BYTE* pBuffer) override {
        if (!m_pStream) return E_FAIL;
        LARGE_INTEGER li;
        li.QuadPart = llPosition;
        HRESULT hr = m_pStream->Seek(li, STREAM_SEEK_SET, NULL);
        if (FAILED(hr)) return hr;
        ULONG cbRead = 0;
        return m_pStream->Read(pBuffer, lLength, &cbRead);
    }

    STDMETHODIMP SyncReadAligned(IMediaSample* pSample) override {
        REFERENCE_TIME tStart, tStop;
        pSample->GetTime(&tStart, &tStop);
        LONGLONG llPos = tStart / 10000000;
        LONG lLen = (LONG)((tStop - tStart) / 10000000);
        BYTE* pBuf = NULL;
        pSample->GetPointer(&pBuf);
        return SyncRead(llPos, lLen, pBuf);
    }
};

// ============================================================
// CVideoFilter impl
// ============================================================
CVideoFilter::CVideoFilter(LPUNKNOWN pUnk, HRESULT* phr)
    : CBaseFilter(FILTER_NAME, pUnk, new CCritSec(), CLSID_VideoFilter)
    , m_fileSize(0)
{
    m_pPin = new CVideoOutputPin(this, m_pLock, phr);
}

CVideoFilter::~CVideoFilter() {
    delete m_pPin;
}

CUnknown* WINAPI CVideoFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT* phr) {
    return new CVideoFilter(pUnk, phr);
}

CBasePin* CVideoFilter::GetPin(int n) {
    return (n == 0) ? m_pPin : NULL;
}

HRESULT CVideoFilter::OpenFile(LPCWSTR pszFileName) {
    HANDLE hFile = CreateFileW(pszFileName, GENERIC_READ,
        FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return HRESULT_FROM_WIN32(GetLastError());

    LARGE_INTEGER fileSize;
    GetFileSizeEx(hFile, &fileSize);
    m_fileSize = fileSize.QuadPart;

    CVideoStream* pStream = new CVideoStream(hFile, m_fileSize);
    LONGLONG dataLength = m_fileSize - PADDING_SIZE;
    m_pPin->SetStream(pStream, dataLength);
    pStream->Release();

    StringCchCopyW(m_szFileName, MAX_PATH, pszFileName);
    return S_OK;
}

// ============================================================
// Registration
// ============================================================
const AMOVIESETUP_MEDIATYPE sudOpPinTypes[] = {
    { &MEDIATYPE_Stream, &MEDIASUBTYPE_NULL }
};

const AMOVIESETUP_PIN sudOpPin[] = {
    { L"Output", FALSE, TRUE, FALSE, FALSE,
      &CLSID_NULL, NULL, ARRAYSIZE(sudOpPinTypes), sudOpPinTypes }
};

const AMOVIESETUP_FILTER sudFilter = {
    &CLSID_VideoFilter, FILTER_NAME, MERIT_UNLIKELY, ARRAYSIZE(sudOpPin), sudOpPin
};

CFactoryTemplate g_Templates[] = {
    { FILTER_NAME, &CLSID_VideoFilter, CVideoFilter::CreateInstance, NULL, &sudFilter }
};
int g_cTemplates = ARRAYSIZE(g_Templates);

STDAPI DllRegisterServer() {
    // רשום את הסיומת .VIDEO
    HKEY hKey;
    RegCreateKeyExW(HKEY_CLASSES_ROOT, L".VIDEO", 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    RegSetValueExW(hKey, NULL, 0, REG_SZ, (BYTE*)L"VideoFile", 20);
    RegCloseKey(hKey);
    return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer() {
    RegDeleteKeyW(HKEY_CLASSES_ROOT, L".VIDEO");
    return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);
BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpReserved) {
    return DllEntryPoint((HINSTANCE)hDllHandle, dwReason, lpReserved);
}
