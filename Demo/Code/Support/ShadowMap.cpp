/**
 * Copyright (C) 2012 Jorge Jimenez (jorge@iryoku.com). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are 
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of the copyright holders.
 */


#include "DXUT.h"
#include "ShadowMap.h"


ID3D10Effect *ShadowMap::effect;
ID3D10InputLayout *ShadowMap::vertexLayout;


void ShadowMap::init(ID3D10Device *device) {
    HRESULT hr;
    if (effect == NULL)
        V(D3DX10CreateEffectFromResource(GetModuleHandle(NULL), L"ShadowMap.fx", NULL, NULL, NULL, "fx_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, device, NULL, NULL, &effect, NULL, NULL));

    const D3D10_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0 }
    };
    UINT numElements = sizeof(layout) / sizeof(D3D10_INPUT_ELEMENT_DESC);

    D3D10_PASS_DESC desc;
    V(effect->GetTechniqueByName("ShadowMap")->GetPassByIndex(0)->GetDesc(&desc));
    V(device->CreateInputLayout(layout, numElements, desc.pIAInputSignature, desc.IAInputSignatureSize, &vertexLayout));
}


void ShadowMap::release() {
    SAFE_RELEASE(effect);
    SAFE_RELEASE(vertexLayout);
}


ShadowMap::ShadowMap(ID3D10Device *device, int width, int height) 
        : device(device) {
    depthStencil = new DepthStencil(device, width, height);
}


ShadowMap::~ShadowMap() {
    SAFE_DELETE(depthStencil);
}


void ShadowMap::begin(const D3DXMATRIX &view, const D3DXMATRIX &projection) {
    HRESULT hr;

    device->IASetInputLayout(vertexLayout);

    device->ClearDepthStencilView(*depthStencil, D3D10_CLEAR_DEPTH, 1.0, 0);

    /**
     * This is for rendering linear values:
     * Check this: http://www.mvps.org/directx/articles/linear_z/linearz.htm
     */
    D3DXMATRIX linearProjection = projection;
    float Q = projection._33;
    float N = -projection._43 / projection._33;
    float F = -N * Q / (1 - Q);
    linearProjection._33 /= F;
    linearProjection._43 /= F;

    V(effect->GetVariableByName("view")->AsMatrix()->SetMatrix((float*) &view));
    V(effect->GetVariableByName("projection")->AsMatrix()->SetMatrix((float*) &linearProjection));
    
    device->OMSetRenderTargets(0, NULL, *depthStencil);

    UINT numViewports = 1;
    device->RSGetViewports(&numViewports, &viewport);
    depthStencil->setViewport();
}


void ShadowMap::setWorldMatrix(const D3DXMATRIX &world) {
    HRESULT hr;
    V(effect->GetVariableByName("world")->AsMatrix()->SetMatrix((float*) &world));
}


void ShadowMap::end() {
    device->RSSetViewports(1, &viewport);
    device->OMSetRenderTargets(0, NULL, NULL);
}


D3DXMATRIX ShadowMap::getViewProjectionTextureMatrix(const D3DXMATRIX &view, const D3DXMATRIX &projection) {
    D3DXMATRIX scale;
    D3DXMatrixScaling(&scale, 0.5f, -0.5f, 1.0f);

    D3DXMATRIX translation;
    D3DXMatrixTranslation(&translation, 0.5f, 0.5f, 0.0f);
    
    return view * projection * scale * translation;
}
