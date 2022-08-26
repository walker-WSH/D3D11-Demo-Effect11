#pragma once
#include <vector>
#include <Windows.h>
#include "d3dDefine.h"
#include "d3dx11effect.h"

#pragma comment(lib, "Effects11d.lib")
#pragma comment(lib, "C:\\Program Files (x86)\\Windows Kits\\8.0\\Lib\\win8\\um\\x86\\D3DCompiler.lib")

struct tShader {
	ID3DX11Effect *m_pFX = NULL;
	ID3DX11EffectTechnique *m_pTech = NULL;
	ID3DX11EffectMatrixVariable *m_pFXWorldViewProj = NULL; // 存储effect中的变量
	ID3DX11EffectShaderResourceVariable *g_ptxDiffuse = NULL;

	ComPtr<ID3D11InputLayout> m_pInputLayout;
	ComPtr<ID3D11Buffer> m_pVertexBuffer;

	//pixel shader
	ComPtr<ID3D11Buffer> m_pPSBuffer;

	ComPtr<ID3D11SamplerState> m_pSampleState;

public:
	bool InitShader(ComPtr<ID3D11Device> pDevice, WCHAR* vsFile, WCHAR* psFile, int vertexSize, int vsBufferSize, int psBufferSize);
};