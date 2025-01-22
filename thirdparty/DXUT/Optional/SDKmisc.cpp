//--------------------------------------------------------------------------------------
// File: SDKmisc.cpp
//
// Various helper functionality that is shared between SDK samples
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=320437
//--------------------------------------------------------------------------------------
#include "dxut.h"
#include "SDKmisc.h"

#include "DDSTextureLoader.h"

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Global/Static Members
//--------------------------------------------------------------------------------------
CDXUTResourceCache &WINAPI DXUTGetGlobalResourceCache()
{
    // Using an accessor function gives control of the construction order
    static CDXUTResourceCache *s_cache = nullptr;
    if (!s_cache)
    {
#if defined(DEBUG) || defined(_DEBUG)
        int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        _CrtSetDbgFlag(flag & ~_CRTDBG_ALLOC_MEM_DF);
#endif
        s_cache = new CDXUTResourceCache;
#if defined(DEBUG) || defined(_DEBUG)
        _CrtSetDbgFlag(flag);
#endif
    }
    return *s_cache;
}

//--------------------------------------------------------------------------------------
// Internal functions forward declarations
//--------------------------------------------------------------------------------------
bool DXUTFindMediaSearchTypicalDirs(_Out_writes_(cchSearch) WCHAR *strSearchPath,
                                    _In_ int cchSearch,
                                    _In_ LPCWSTR strLeaf,
                                    _In_ const WCHAR *strExePath,
                                    _In_ const WCHAR *strExeName);
bool DXUTFindMediaSearchParentDirs(_Out_writes_(cchSearch) WCHAR *strSearchPath,
                                   _In_ int cchSearch,
                                   _In_ const WCHAR *strStartAt,
                                   _In_ const WCHAR *strLeafName);

//--------------------------------------------------------------------------------------
// Returns pointer to static media search buffer
//--------------------------------------------------------------------------------------
WCHAR *DXUTMediaSearchPath()
{
    static WCHAR s_strMediaSearchPath[MAX_PATH] =
        {
            0};
    return s_strMediaSearchPath;
}

//--------------------------------------------------------------------------------------
LPCWSTR WINAPI DXUTGetMediaSearchPath()
{
    return DXUTMediaSearchPath();
}

//--------------------------------------------------------------------------------------
HRESULT WINAPI DXUTSetMediaSearchPath(_In_z_ LPCWSTR strPath)
{
    HRESULT hr;

    WCHAR *s_strSearchPath = DXUTMediaSearchPath();

    hr = wcscpy_s(s_strSearchPath, MAX_PATH, strPath);
    if (SUCCEEDED(hr))
    {
        // append slash if needed
        size_t ch = 0;
        ch = wcsnlen(s_strSearchPath, MAX_PATH);
        if (SUCCEEDED(hr) && s_strSearchPath[ch - 1] != L'\\')
        {
            hr = wcscat_s(s_strSearchPath, MAX_PATH, L"\\");
        }
    }

    return hr;
}

//--------------------------------------------------------------------------------------
// Tries to find the location of a SDK media file
//       cchDest is the size in WCHARs of strDestPath.  Be careful not to
//       pass in sizeof(strDest) on UNICODE builds.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
    HRESULT WINAPI
    DXUTFindDXSDKMediaFileCch(WCHAR *strDestPath, int cchDest,
                              LPCWSTR strFilename)
{
    bool bFound;
    WCHAR strSearchFor[MAX_PATH];

    if (!strFilename || strFilename[0] == 0 || !strDestPath || cchDest < 10)
        return E_INVALIDARG;

    // Get the exe name, and exe path
    WCHAR strExePath[MAX_PATH] =
        {
            0};
    WCHAR strExeName[MAX_PATH] =
        {
            0};
    WCHAR *strLastSlash = nullptr;
    GetModuleFileName(nullptr, strExePath, MAX_PATH);
    strExePath[MAX_PATH - 1] = 0;
    strLastSlash = wcsrchr(strExePath, TEXT('\\'));
    if (strLastSlash)
    {
        wcscpy_s(strExeName, MAX_PATH, &strLastSlash[1]);

        // Chop the exe name from the exe path
        *strLastSlash = 0;

        // Chop the .exe from the exe name
        strLastSlash = wcsrchr(strExeName, TEXT('.'));
        if (strLastSlash)
            *strLastSlash = 0;
    }

    // Typical directories:
    //      .\
    //      ..\
    //      ..\..\
    //      %EXE_DIR%\
    //      %EXE_DIR%\..\
    //      %EXE_DIR%\..\..\
    //      %EXE_DIR%\..\%EXE_NAME%
    //      %EXE_DIR%\..\..\%EXE_NAME%

    // Typical directory search
    bFound = DXUTFindMediaSearchTypicalDirs(strDestPath, cchDest, strFilename, strExePath, strExeName);
    if (bFound)
        return S_OK;

    // Typical directory search again, but also look in a subdir called "\media\"
    swprintf_s(strSearchFor, MAX_PATH, L"media\\%ls", strFilename);
    bFound = DXUTFindMediaSearchTypicalDirs(strDestPath, cchDest, strSearchFor, strExePath, strExeName);
    if (bFound)
        return S_OK;

    WCHAR strLeafName[MAX_PATH] =
        {
            0};

    // Search all parent directories starting at .\ and using strFilename as the leaf name
    wcscpy_s(strLeafName, MAX_PATH, strFilename);
    bFound = DXUTFindMediaSearchParentDirs(strDestPath, cchDest, L".", strLeafName);
    if (bFound)
        return S_OK;

    // Search all parent directories starting at the exe's dir and using strFilename as the leaf name
    bFound = DXUTFindMediaSearchParentDirs(strDestPath, cchDest, strExePath, strLeafName);
    if (bFound)
        return S_OK;

    // Search all parent directories starting at .\ and using "media\strFilename" as the leaf name
    swprintf_s(strLeafName, MAX_PATH, L"media\\%ls", strFilename);
    bFound = DXUTFindMediaSearchParentDirs(strDestPath, cchDest, L".", strLeafName);
    if (bFound)
        return S_OK;

    // Search all parent directories starting at the exe's dir and using "media\strFilename" as the leaf name
    bFound = DXUTFindMediaSearchParentDirs(strDestPath, cchDest, strExePath, strLeafName);
    if (bFound)
        return S_OK;

    // On failure, return the file as the path but also return an error code
    wcscpy_s(strDestPath, cchDest, strFilename);

    return DXUTERR_MEDIANOTFOUND;
}

//--------------------------------------------------------------------------------------
// Search a set of typical directories
//--------------------------------------------------------------------------------------
_Use_decl_annotations_ bool DXUTFindMediaSearchTypicalDirs(WCHAR *strSearchPath, int cchSearch, LPCWSTR strLeaf,
                                                           const WCHAR *strExePath, const WCHAR *strExeName)
{
    // Typical directories:
    //      .\
    //      ..\
    //      ..\..\
    //      %EXE_DIR%\
    //      %EXE_DIR%\..\
    //      %EXE_DIR%\..\..\
    //      %EXE_DIR%\..\%EXE_NAME%
    //      %EXE_DIR%\..\..\%EXE_NAME%
    //      DXSDK media path

    // Search in .\  
    wcscpy_s( strSearchPath, cchSearch, strLeaf );
    if (GetFileAttributes(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in ..\  
    swprintf_s( strSearchPath, cchSearch, L"..\\%ls", strLeaf );
    if (GetFileAttributes(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in ..\..\ 
    swprintf_s( strSearchPath, cchSearch, L"..\\..\\%ls", strLeaf );
    if (GetFileAttributes(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in ..\..\ 
    swprintf_s( strSearchPath, cchSearch, L"..\\..\\%ls", strLeaf );
    if (GetFileAttributes(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in the %EXE_DIR%\ 
    swprintf_s( strSearchPath, cchSearch, L"%ls\\%ls", strExePath, strLeaf );
    if (GetFileAttributes(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in the %EXE_DIR%\..\ 
    swprintf_s( strSearchPath, cchSearch, L"%ls\\..\\%ls", strExePath, strLeaf );
    if (GetFileAttributes(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in the %EXE_DIR%\..\..\ 
    swprintf_s( strSearchPath, cchSearch, L"%ls\\..\\..\\%ls", strExePath, strLeaf );
    if (GetFileAttributes(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in "%EXE_DIR%\..\%EXE_NAME%\".  This matches the DirectX SDK layout
    swprintf_s(strSearchPath, cchSearch, L"%ls\\..\\%ls\\%ls", strExePath, strExeName, strLeaf);
    if (GetFileAttributes(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in "%EXE_DIR%\..\..\%EXE_NAME%\".  This matches the DirectX SDK layout
    swprintf_s(strSearchPath, cchSearch, L"%ls\\..\\..\\%ls\\%ls", strExePath, strExeName, strLeaf);
    if (GetFileAttributes(strSearchPath) != 0xFFFFFFFF)
        return true;

    // Search in media search dir
    WCHAR *s_strSearchPath = DXUTMediaSearchPath();
    if (s_strSearchPath[0] != 0)
    {
        swprintf_s(strSearchPath, cchSearch, L"%ls%ls", s_strSearchPath, strLeaf);
        if (GetFileAttributes(strSearchPath) != 0xFFFFFFFF)
            return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------
// Search parent directories starting at strStartAt, and appending strLeafName
// at each parent directory.  It stops at the root directory.
//--------------------------------------------------------------------------------------
_Use_decl_annotations_ bool DXUTFindMediaSearchParentDirs(WCHAR *strSearchPath, int cchSearch, const WCHAR *strStartAt,
                                                          const WCHAR *strLeafName)
{
    WCHAR strFullPath[MAX_PATH] =
        {
            0};
    WCHAR strFullFileName[MAX_PATH] =
        {
            0};
    WCHAR strSearch[MAX_PATH] =
        {
            0};
    WCHAR *strFilePart = nullptr;

    if (!GetFullPathName(strStartAt, MAX_PATH, strFullPath, &strFilePart))
        return false;

#pragma warning(disable : 6102)
    while (strFilePart && *strFilePart != '\0')
    {
        swprintf_s(strFullFileName, MAX_PATH, L"%ls\\%ls", strFullPath, strLeafName);
        if (GetFileAttributes(strFullFileName) != 0xFFFFFFFF)
        {
            wcscpy_s(strSearchPath, cchSearch, strFullFileName);
            return true;
        }

        swprintf_s(strSearch, MAX_PATH, L"%ls\\..", strFullPath);
        if (!GetFullPathName(strSearch, MAX_PATH, strFullPath, &strFilePart))
            return false;
    }

    return false;
}

//--------------------------------------------------------------------------------------
// Compiles HLSL shaders
//--------------------------------------------------------------------------------------
#if D3D_COMPILER_VERSION < 46

namespace
{

    struct handle_closer
    {
        void operator()(HANDLE h)
        {
            if (h)
                CloseHandle(h);
        }
    };

    typedef std::unique_ptr<void, handle_closer> ScopedHandle;

    inline HANDLE safe_handle(HANDLE h) { return (h == INVALID_HANDLE_VALUE) ? nullptr : h; }

    class CIncludeHandler : public ID3DInclude
    // Not as robust as D3D_COMPILE_STANDARD_FILE_INCLUDE, but it works in most cases
    {
    private:
        static const unsigned int MAX_INCLUDES = 9;

        struct sInclude
        {
            HANDLE hFile;
            HANDLE hFileMap;
            LARGE_INTEGER FileSize;
            void *pMapData;
        };

        struct sInclude m_includeFiles[MAX_INCLUDES];
        size_t m_nIncludes;
        bool m_reset;
        WCHAR m_workingPath[MAX_PATH];

    public:
        CIncludeHandler() : m_nIncludes(0), m_reset(false)
        {
            if (!GetCurrentDirectoryW(MAX_PATH, m_workingPath))
                *m_workingPath = 0;

            for (size_t i = 0; i < MAX_INCLUDES; ++i)
            {
                m_includeFiles[i].hFile = INVALID_HANDLE_VALUE;
                m_includeFiles[i].hFileMap = INVALID_HANDLE_VALUE;
                m_includeFiles[i].pMapData = nullptr;
            }
        }
        virtual ~CIncludeHandler()
        {
            for (size_t i = 0; i < m_nIncludes; ++i)
            {
                UnmapViewOfFile(m_includeFiles[i].pMapData);

                if (m_includeFiles[i].hFileMap != INVALID_HANDLE_VALUE)
                    CloseHandle(m_includeFiles[i].hFileMap);

                if (m_includeFiles[i].hFile != INVALID_HANDLE_VALUE)
                    CloseHandle(m_includeFiles[i].hFile);
            }

            m_nIncludes = 0;

            if (m_reset && *m_workingPath)
            {
                SetCurrentDirectoryW(m_workingPath);
            }
        }

        STDMETHOD(Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes))
        {
            UNREFERENCED_PARAMETER(IncludeType);
            UNREFERENCED_PARAMETER(pParentData);

            size_t incIndex = m_nIncludes + 1;

            // Make sure we have enough room for this include file
            if (incIndex >= MAX_INCLUDES)
                return E_FAIL;

            // try to open the file
            m_includeFiles[incIndex].hFile = CreateFileA(pFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
            if (INVALID_HANDLE_VALUE == m_includeFiles[incIndex].hFile)
            {
                return E_FAIL;
            }

            // Get the file size
            GetFileSizeEx(m_includeFiles[incIndex].hFile, &m_includeFiles[incIndex].FileSize);

            // Use Memory Mapped File I/O for the header data
            m_includeFiles[incIndex].hFileMap = CreateFileMappingA(m_includeFiles[incIndex].hFile, nullptr, PAGE_READONLY, m_includeFiles[incIndex].FileSize.HighPart, m_includeFiles[incIndex].FileSize.LowPart, pFileName);
            if (!m_includeFiles[incIndex].hFileMap)
            {
                if (m_includeFiles[incIndex].hFile != INVALID_HANDLE_VALUE)
                    CloseHandle(m_includeFiles[incIndex].hFile);
                return E_FAIL;
            }

            // Create Map view
            *ppData = MapViewOfFile(m_includeFiles[incIndex].hFileMap, FILE_MAP_READ, 0, 0, 0);
            *pBytes = m_includeFiles[incIndex].FileSize.LowPart;

            // Success - Increment the include file count
            m_nIncludes = incIndex;

            return S_OK;
        }

        STDMETHOD(Close(LPCVOID pData))
        {
            UNREFERENCED_PARAMETER(pData);
            // Defer Closure until the container destructor
            return S_OK;
        }

        void SetCWD(LPCWSTR pFileName)
        {
            WCHAR filePath[MAX_PATH];
            wcscpy_s(filePath, MAX_PATH, pFileName);

            WCHAR *strLastSlash = wcsrchr(filePath, L'\\');
            if (strLastSlash)
            {
                // Chop the exe name from the exe path
                *strLastSlash = 0;
                m_reset = true;
                SetCurrentDirectoryW(filePath);
            }
        }
    };

}; // namespace

#endif

//--------------------------------------------------------------------------------------
// Texture utilities
//--------------------------------------------------------------------------------------

_Use_decl_annotations_
    HRESULT WINAPI
    DXUTCreateShaderResourceViewFromFile(ID3D11Device *d3dDevice, const wchar_t *szFileName, ID3D11ShaderResourceView **textureView)
{
    if (!d3dDevice || !szFileName || !textureView)
        return E_INVALIDARG;

    WCHAR str[MAX_PATH];
    HRESULT hr = DXUTFindDXSDKMediaFileCch(str, MAX_PATH, szFileName);
    if (FAILED(hr))
        return hr;

    WCHAR ext[_MAX_EXT];
    _wsplitpath_s(str, nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT);

    hr = DirectX::CreateDDSTextureFromFile(d3dDevice, str, nullptr, textureView);

    return hr;
}

_Use_decl_annotations_
    HRESULT WINAPI
    DXUTCreateTextureFromFile(ID3D11Device *d3dDevice, const wchar_t *szFileName, ID3D11Resource **texture)
{
    if (!d3dDevice || !szFileName || !texture)
        return E_INVALIDARG;

    WCHAR str[MAX_PATH];
    HRESULT hr = DXUTFindDXSDKMediaFileCch(str, MAX_PATH, szFileName);
    if (FAILED(hr))
        return hr;

    WCHAR ext[_MAX_EXT];
    _wsplitpath_s(str, nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT);

    hr = DirectX::CreateDDSTextureFromFile(d3dDevice, str, texture, nullptr);

    return hr;
}

//======================================================================================
// CDXUTResourceCache
//======================================================================================

CDXUTResourceCache::~CDXUTResourceCache()
{
    OnDestroyDevice();
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
    HRESULT
    CDXUTResourceCache::CreateTextureFromFile(ID3D11Device *pDevice, LPCWSTR pSrcFile,
                                              ID3D11ShaderResourceView **ppOutputRV, bool bSRGB)
{
    if (!ppOutputRV)
        return E_INVALIDARG;

    *ppOutputRV = nullptr;

    for (auto it = m_TextureCache.cbegin(); it != m_TextureCache.cend(); ++it)
    {
        if (!wcscmp(it->wszSource, pSrcFile) && it->bSRGB == bSRGB && it->pSRV11)
        {
            it->pSRV11->AddRef();
            *ppOutputRV = it->pSRV11;
            return S_OK;
        }
    }

    WCHAR ext[_MAX_EXT];
    _wsplitpath_s(pSrcFile, nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT);

    HRESULT hr = DirectX::CreateDDSTextureFromFileEx(pDevice, pSrcFile, 0,
                                                     D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, bSRGB,
                                                     nullptr, ppOutputRV, nullptr);

    if (FAILED(hr))
        return hr;

    DXUTCache_Texture entry;
    wcscpy_s(entry.wszSource, MAX_PATH, pSrcFile);
    entry.bSRGB = bSRGB;
    entry.pSRV11 = *ppOutputRV;
    entry.pSRV11->AddRef();
    m_TextureCache.push_back(entry);

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Device event callbacks
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
HRESULT CDXUTResourceCache::OnDestroyDevice()
{
    // Release all resources
    for (size_t j = 0; j < m_TextureCache.size(); ++j)
    {
        SAFE_RELEASE(m_TextureCache[j].pSRV11);
    }
    m_TextureCache.clear();
    m_TextureCache.shrink_to_fit();

    return S_OK;
}
