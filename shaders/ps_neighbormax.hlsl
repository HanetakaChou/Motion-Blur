//----------------------------------------------------------------------------------
// File:        MotionBlurAdvanced\assets\shaders/ps_neighbormax.hlsl
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


#include "constants.hlsli"



////////////////////////////////////////////////////////////////////////////////

// Resources



Texture2D texTileMax : register(t0);



////////////////////////////////////////////////////////////////////////////////

// IO Structures



struct VS_OUTPUT

{

	float4 P  : SV_POSITION;

	float2 TC : TEXCOORD0;

};



////////////////////////////////////////////////////////////////////////////////

// Pixel Shader



float4 main(VS_OUTPUT input) : SV_Target0

{

	float4 vOutputColor = GRAY;



	float2 texCoordBase = input.TC;

	float2 texCoordIncrement = float2(1, 1) / textureSize(texTileMax);

	float fMaxMagnitudeSquared = 0.0;

	for (int s = -1; s <= 1; ++s)

	{

		for (int t = -1; t <= 1; ++t)

		{

			float2 texCoords = texCoordBase + (float2(s, t) * texCoordIncrement);

			float2 texLookup = texTileMax.SampleLevel(sampPointClamp, texCoords, 0).xy;

			float2 vVelocity = readBiasScale(texLookup);



			float fMagnitudeSquared = dot(vVelocity, vVelocity);

			if (fMaxMagnitudeSquared < fMagnitudeSquared)

			{

				float  fDisplacement = abs(float(s)) + abs(float(t));

				float2 vOrientation = sign(float2(s, t) * vVelocity);

				float  fDistance = vOrientation.x + vOrientation.y;



				if (abs(fDistance) == fDisplacement)

				{

					vOutputColor.xy = writeBiasScale(vVelocity);

					fMaxMagnitudeSquared = fMagnitudeSquared;

				}

			}

		}

	}

	return vOutputColor;

}

