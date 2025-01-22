//----------------------------------------------------------------------------------
// File:        MotionBlurAdvanced\src/scene.cpp
// SDK Version: v1.2
// Email:       gameworks@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2014, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------------

#include "common_util.h"
#include <vector>
#include <map>
#include "scene.h"
#include <DDSTextureLoader.h>
#include <SDKmisc.h>
#include <string>

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{

    ////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////

}
namespace Scene
{

    ////////////////////////////////////////////////////////////////////////////////////////////////////

    HRESULT load_texture(ID3D11Device *device, wchar_t const *filename, ID3D11ShaderResourceView **out_srv)
    {
        WCHAR str[MAX_PATH];
        HRESULT hr = DXUTFindDXSDKMediaFileCch(str, MAX_PATH, filename);
        if (FAILED(hr))
            return hr;

        hr = DXUTGetGlobalResourceCache().CreateTextureFromFile(device, str, out_srv);
        return hr;
    }

    HRESULT load_model(ID3D11Device *device, unsigned int num_faces, unsigned int const *indices, unsigned int num_vertices, float const *vertices, float const *normals, float const *texture_coords, wchar_t const *diffuse_texture_filename, wchar_t const *normal_texture_filename, std::vector<RenderObject *> &out_objects)

    {
        out_objects.push_back(new RenderObject(device, num_faces, indices, num_vertices, vertices, normals, texture_coords, diffuse_texture_filename, normal_texture_filename));

        return S_OK;
    }

    RenderObject::RenderObject(ID3D11Device *device, unsigned int num_faces, unsigned int const *indices, unsigned int num_vertices, float const *vertices, float const *normals, float const *texture_coords, wchar_t const *diffuse_texture_filename, wchar_t const *normal_texture_filename)
    {
        HRESULT hr;

        this->idx_count = num_faces * 3;

        this->idx_offset = 0;

        UINT idx_size = sizeof(UINT);

        UINT idx_buffer_size = idx_size * this->idx_count;

        UINT *source_indices = new UINT[this->idx_count];

        for (UINT idx_idx = 0; idx_idx < this->idx_count; ++idx_idx)
        {
            source_indices[idx_idx] = indices[idx_idx];
        }

        D3D11_BUFFER_DESC ib_desc = {

            idx_buffer_size, // Byte Width

            D3D11_USAGE_DEFAULT, // Usage

            D3D11_BIND_INDEX_BUFFER, // Bind Flags

            0, // CPU Access

            0, // Misc Flags

            0, // Structure Byte Stride

        };

        D3D11_SUBRESOURCE_DATA ib_data = {

            (void *)source_indices, // Source Data

            0, // Line Pitch

            0 // Slice Pitch

        };

        device->CreateBuffer(&ib_desc, &ib_data, &this->idx_buffer);

        delete[] source_indices;

        this->vtx_count = num_vertices;

        this->vtx_offset = 0;

        void *source_data[MAX_VTX_BUFFERS] = {nullptr};

        UINT source_strides[MAX_VTX_BUFFERS];

        source_data[0] = (void *)vertices;

        source_strides[0] = sizeof(float) * 3U;

        source_data[1] = (void *)normals;

        source_strides[1] = sizeof(float) * 3U;

        source_data[2] = (void *)texture_coords;

        source_strides[2] = sizeof(float) * 2U;

        this->vtx_layout.key = 0;

        this->vtx_layout.desc.position = VTXLAYOUTDESC_POSITION_FLOAT3;

        this->vtx_layout.desc.normal = VTXLAYOUTDESC_NBT_FLOAT3;

        this->vtx_layout.desc.tc_count = 1;

        this->vtx_layout.desc.tc0 = VTXLAYOUTDESC_TEXCOORD_FLOAT2;

        this->vtx_buffer_count = 3;

        for (UINT idx = 0; idx < MAX_VTX_BUFFERS; ++idx)
        {
            this->vtx_buffers[idx] = nullptr;
        }

        for (UINT buffer_idx = 0; buffer_idx < this->vtx_buffer_count; ++buffer_idx)
        {
            UINT element_size = source_strides[buffer_idx];

            UINT buffer_size = element_size * this->vtx_count;

            this->vtx_strides[buffer_idx] = source_strides[buffer_idx];

            this->vtx_offsets[buffer_idx] = 0;

            D3D11_BUFFER_DESC vb_desc = {

                buffer_size, // Byte Width

                D3D11_USAGE_DEFAULT, // Usage

                D3D11_BIND_VERTEX_BUFFER, // Bind Flags

                0, // CPU Access

                0, // Misc Flags

                0, // Structure Byte Stride

            };

            D3D11_SUBRESOURCE_DATA vb_data = {

                source_data[buffer_idx], // Source Data

                0, // Line Pitch

                0 // Slice Pitch

            };

            device->CreateBuffer(&vb_desc, &vb_data, &this->vtx_buffers[buffer_idx]);
        }

        {
            ID3D11ShaderResourceView *srv = NULL;

            hr = load_texture(device, diffuse_texture_filename, &srv);

            _ASSERT(SUCCEEDED(hr));

            this->material_properties[Scene::DIFFUSE_TEX] = (void *)srv;
        }

        {
            ID3D11ShaderResourceView *srv = NULL;

            hr = load_texture(device, normal_texture_filename, &srv);

            _ASSERT(SUCCEEDED(hr));

            this->material_properties[Scene::NORMAL_TEX] = (void *)srv;
        }
    }

    RenderObject::~RenderObject()

    {

        SAFE_RELEASE(this->idx_buffer);

        for (int i = 0; i < MAX_VTX_BUFFERS; ++i)

        {

            if (this->vtx_buffers[i])

            {

                SAFE_RELEASE(this->vtx_buffers[i]);
            }
        }

        for (auto prop = this->material_properties.begin(); prop != this->material_properties.end(); ++prop)

        {

            ID3D11ShaderResourceView *srv = (ID3D11ShaderResourceView *)(*prop).second;

            SAFE_RELEASE(srv);
        }
    }

    void RenderObject::render(ID3D11DeviceContext *ctx)

    {

        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ctx->IASetVertexBuffers(0, this->vtx_buffer_count, this->vtx_buffers, this->vtx_strides, this->vtx_offsets);

        ctx->IASetIndexBuffer(this->idx_buffer, DXGI_FORMAT_R32_UINT, this->idx_offset);

        ID3D11ShaderResourceView *srvs[16];

        srvs[0] = (ID3D11ShaderResourceView *)material_properties[Scene::DIFFUSE_TEX];

        srvs[1] = (ID3D11ShaderResourceView *)material_properties[Scene::SPECULAR_TEX];

        srvs[2] = (ID3D11ShaderResourceView *)material_properties[Scene::NORMAL_TEX];

        ctx->PSSetShaderResources(0, 3, srvs);

        ctx->DrawIndexed(this->idx_count, this->idx_offset, this->vtx_offset);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////

}