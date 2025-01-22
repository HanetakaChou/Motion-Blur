//--------------------------------------------------------------------------------------
// File: SDKMisc.h
//
// Various helper functionality that is shared between SDK samples
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=320437
//--------------------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
// Resource cache for textures, fonts, meshs, and effects.
// Use DXUTGetGlobalResourceCache() to access the global cache
//-----------------------------------------------------------------------------

struct DXUTCache_Texture
{
    WCHAR wszSource[MAX_PATH];
    bool bSRGB;
    ID3D11ShaderResourceView *pSRV11;

    DXUTCache_Texture() noexcept : wszSource{},
                                   bSRGB(false),
                                   pSRV11(nullptr)
    {
    }
};

class CDXUTResourceCache
{
public:
    ~CDXUTResourceCache();

    HRESULT CreateTextureFromFile(_In_ ID3D11Device *pDevice, _In_z_ LPCWSTR pSrcFile,
                                  _Outptr_ ID3D11ShaderResourceView **ppOutputRV, _In_ bool bSRGB = false);

public:
    HRESULT OnDestroyDevice();

protected:
    friend CDXUTResourceCache &WINAPI DXUTGetGlobalResourceCache();

    CDXUTResourceCache() = default;

    std::vector<DXUTCache_Texture> m_TextureCache;
};

CDXUTResourceCache &WINAPI DXUTGetGlobalResourceCache();

//--------------------------------------------------------------------------------------
// Tries to finds a media file by searching in common locations
//--------------------------------------------------------------------------------------
HRESULT WINAPI DXUTFindDXSDKMediaFileCch(_Out_writes_(cchDest) WCHAR *strDestPath,
                                         _In_ int cchDest,
                                         _In_z_ LPCWSTR strFilename);
HRESULT WINAPI DXUTSetMediaSearchPath(_In_z_ LPCWSTR strPath);
LPCWSTR WINAPI DXUTGetMediaSearchPath();

//--------------------------------------------------------------------------------------
// Texture utilities
//--------------------------------------------------------------------------------------
HRESULT WINAPI DXUTCreateShaderResourceViewFromFile(_In_ ID3D11Device *d3dDevice, _In_z_ const wchar_t *szFileName, _Outptr_ ID3D11ShaderResourceView **textureView);
HRESULT WINAPI DXUTCreateTextureFromFile(_In_ ID3D11Device *d3dDevice, _In_z_ const wchar_t *szFileName, _Outptr_ ID3D11Resource **texture);
