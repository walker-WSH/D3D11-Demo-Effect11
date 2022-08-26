#include "stdafx.h"
#include "RenderCenter.h"

static D3D_DRIVER_TYPE driverTypes[] = {
	D3D_DRIVER_TYPE_HARDWARE,
	D3D_DRIVER_TYPE_WARP,
	D3D_DRIVER_TYPE_REFERENCE,
};

static D3D_FEATURE_LEVEL featureLevels[] = {
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_9_3,
};

//----------------------------------------------
CRenderCenter::CRenderCenter() : m_pTextureShader(0) {}

CRenderCenter::~CRenderCenter() {}

bool CRenderCenter::InitRender(HWND hPreview)
{
	UninitRender();

	if (!_InitDevice())
		return false;

	if (!_InitBlendState())
		return false;

	if (!_AddSwapChain(hPreview)) // create default display
		return false;

	if (!m_ShaderRGBA.InitShader(m_pDevice, L"shader\\texture.vs", L"shader\\texture.vs", sizeof(tTextureVertexType) * TEXTURE_VERTEX_COUNT, sizeof(D3DXMATRIX), 0)) {
		return false;
	}

	return true;
}

void CRenderCenter::UninitRender()
{
	m_WndList.clear();
}

bool CRenderCenter::_InitBlendState()
{
	D3D11_BLEND_DESC blendStateDescription;
	ZeroMemory(&blendStateDescription, sizeof(D3D11_BLEND_DESC));

	blendStateDescription.RenderTarget[0].BlendEnable = TRUE;

	blendStateDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDescription.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendStateDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;

	blendStateDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	if (FAILED(m_pDevice->CreateBlendState(&blendStateDescription, m_pBlendState.Assign())))
		return false;

	return true;
}

void CRenderCenter::_SetDisplayWnd(tWindowSwap &swap)
{
	ID3D11RenderTargetView *view = swap.m_pRenderTargetView.Get();
	m_pDeviceContext->OMSetRenderTargets(1, &view, NULL);

	CRect rcClient;
	::GetClientRect(swap.m_hWnd, &rcClient);

	D3D11_VIEWPORT vp;
	memset(&vp, 0, sizeof(vp));
	vp.MinDepth = 0.f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = (float)0;
	vp.TopLeftY = (float)0;
	vp.Width = (float)rcClient.Width();
	vp.Height = (float)rcClient.Height();
	m_pDeviceContext->RSSetViewports(1, &vp);
}

bool CRenderCenter::_InitDevice()
{
	HRESULT hr = S_OK;

	UINT numDriverTypes = ARRAYSIZE(driverTypes);
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);
	D3D_FEATURE_LEVEL levelUsed = D3D_FEATURE_LEVEL_9_3;
	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++) {
		hr = D3D11CreateDevice(nullptr, driverTypes[driverTypeIndex], nullptr, 0, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, m_pDevice.Assign(), &levelUsed,
				       m_pDeviceContext.Assign());

		if (SUCCEEDED(hr))
			break;
	}

	if (FAILED(hr))
		return false;

	ComPtr<IDXGIDevice> dxgiDevice;
	hr = m_pDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void **>(dxgiDevice.Assign()));
	if (SUCCEEDED(hr)) {
		ComPtr<IDXGIAdapter> adapter;
		hr = dxgiDevice->GetAdapter(adapter.Assign());
		if (SUCCEEDED(hr)) {
			DXGI_ADAPTER_DESC adapterDesc = {};
			adapter->GetDesc(&adapterDesc);
			m_strAdapterName = adapterDesc.Description;

			hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void **>(m_pFactory1.Assign()));
		}
	}

	if (FAILED(hr))
		return false;

	return true;
}

bool CRenderCenter::_AddSwapChain(HWND hWnd)
{
	tWindowSwap swap;
	if (!swap.InitSwap(m_pFactory1, m_pDevice, hWnd))
		return false;

	m_WndList.push_back(swap);
	m_nCurrentSwap = m_WndList.size() - 1;

	_SetDisplayWnd(m_WndList[m_nCurrentSwap]);
	return true;
}

D3DXMATRIX CRenderCenter::_GetOrthoMatrix(int baseWidth, int baseHeight)
{
	FLOAT zn = -100.f;
	FLOAT zf = 100.f;
	D3DXMATRIX orthoMatrix;

	float left = 0;
	float right = baseWidth;
	float top = 0;
	float bottom = baseHeight;

	if (1) {
		// 方法一：
		memset(orthoMatrix.m, 0, sizeof(orthoMatrix.m));

		orthoMatrix.m[0][0] = 2 / ((float)baseWidth);
		orthoMatrix.m[1][1] = 2 / (-1.0f * baseHeight);
		orthoMatrix.m[2][2] = 1 / (float)(zf - zn);
		orthoMatrix.m[3][0] = -1.0f;
		orthoMatrix.m[3][1] = 1.0f;
		orthoMatrix.m[3][2] = 0.5f;
		orthoMatrix.m[3][3] = 1.0f;
		 
	} else {
		// 方法二：
		D3DXMatrixOrthoLH(&orthoMatrix, (float)baseWidth, (float)baseHeight, zn, zf);
		orthoMatrix.m[1][1] = -orthoMatrix.m[1][1];
		orthoMatrix.m[3][0] = -1.0f;
		orthoMatrix.m[3][1] = 1.0f;
	}

	return orthoMatrix;
}

void CRenderCenter::BeginRender()
{
	tWindowSwap &swap = m_WndList[m_nCurrentSwap];
	if (swap.TestResizeSwapChain(m_pDevice, m_pDeviceContext))
		_SetDisplayWnd(swap);

	float blendFactor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	m_pDeviceContext->OMSetBlendState(m_pBlendState, blendFactor, 0xffffffff);

	float color[4] = {1.f, 1.f, 1.f, 1.f};
	m_pDeviceContext->ClearRenderTargetView(m_WndList[m_nCurrentSwap].m_pRenderTargetView, color);
}

void CRenderCenter::EndRender()
{
	m_WndList[m_nCurrentSwap].m_pSwapChain->Present(0, 0);
}

void CRenderCenter::PrepareRenderTexture(tTexture *pTexture)
{
	switch (pTexture->m_ShaderType) {
	case EST_YUYV422: {
		float wh[2] = {(float)pTexture->m_nWidth, (float)pTexture->m_nHeight};
		m_pDeviceContext->UpdateSubresource(m_ShaderYUYV422.m_pPSBuffer, 0, nullptr, wh, 0, 0);
	}
		m_pTextureShader = &m_ShaderYUYV422;
		break;

	case EST_YUV420:
		m_pTextureShader = &m_ShaderYUV420;
		break;

	case EST_YUV420_Ex:
		m_pTextureShader = &m_ShaderYUV420_Ex;
		break;

	case EST_DEFAULT:
	default:
		m_pTextureShader = &m_ShaderRGBA;
		break;
	}
}

void CRenderCenter::UpdateMatrix(D3DXMATRIX worldMatrix, int baseWidth, int baseHeight)
{
	worldMatrix.m[0][2] = -worldMatrix.m[0][2];
	worldMatrix.m[1][2] = -worldMatrix.m[1][2];
	worldMatrix.m[2][2] = -worldMatrix.m[2][2];
	worldMatrix.m[3][2] = -worldMatrix.m[3][2];

	D3DXMATRIX orthoMatrix = _GetOrthoMatrix(baseWidth, baseHeight);

	D3DXMatrixIdentity(&m_wvpMatrix);
	m_wvpMatrix = worldMatrix * orthoMatrix;
	D3DXMatrixTranspose(&m_wvpMatrix, &m_wvpMatrix);

	m_pTextureShader->m_pFXWorldViewProj->SetMatrixTranspose(&(m_wvpMatrix.m[0][0]));
	//m_pTextureShader->m_pFXWorldViewProj->SetRawValue(&(m_wvpMatrix.m[0][0]), 0, sizeof(m_wvpMatrix));
}

void SwapFloat(float &f1, float &f2)
{
	float temp = f1;
	f1 = f2;
	f2 = temp;
}

void CRenderCenter::_UpdateTextureVertexBuffer(int w, int h, bool bFlipH, bool bFlipV)
{
	float left = 0;
	float right = left + (float)w;
	float top = 0;
	float bottom = top + (float)h;

	//float leftUV = 0.f;
	//float rightUV = 1.f;
	//float topUV = 0.f;
	//float bottomUV = 1.f;
	 
	float leftUV = 0.f;
	float rightUV = 1.f;
	float topUV = 0.f;
	float bottomUV = 1.f;

	if (bFlipH)
		SwapFloat(leftUV, rightUV);

	if (bFlipV)
		SwapFloat(topUV, bottomUV);

	tTextureVertexType vertex[TEXTURE_VERTEX_COUNT];
	vertex[0] = {left, top, 0, 1.f, leftUV, topUV};
	vertex[1] = {right, top, 0, 1.f, rightUV, topUV};
	vertex[2] = {left, bottom, 0, 1.f, leftUV, bottomUV};
	vertex[3] = {right, bottom, 0, 1.f, rightUV, bottomUV};

	m_pDeviceContext->UpdateSubresource(m_pTextureShader->m_pVertexBuffer, 0, nullptr, &vertex, 0, 0);
}

void CRenderCenter::RenderTexture(tTexture *pTexture)
{
	int viewCount;
	ID3D11ShaderResourceView *out[MAX_TEXTURE_NUM] = {};
	pTexture->GetResourceViewList(out, viewCount);

	_RenderTextureInner(out, viewCount, pTexture);
}

ID3D11Texture2D *GetTexture(ID3D11View *m_pResourceView)
{
	ID3D11Resource *res = NULL;
	ID3D11Texture2D *pTextureInterface = NULL;

	m_pResourceView->GetResource(&res); // ID3D11ShaderResourceView
	if (res) {
		res->QueryInterface<ID3D11Texture2D>(&pTextureInterface);
		res->Release();
	}

	return pTextureInterface;
}

void SaveBitmapFile(char *path, uint8_t *data, int linesize, int width, int height, int per_pixel_byte, bool flip);
void SaveTextureAsFile(ID3D11Device *dev, ID3D11DeviceContext *ctx, ID3D11Texture2D *pD3DTextureSrc, char *path);
void CRenderCenter::_RenderTextureInner(ID3D11ShaderResourceView **views, int count, tTexture *pTexture)
{
	unsigned int stride = sizeof(tTextureVertexType);
	unsigned int offset = 0;
	ID3D11Buffer *buffer[1];

	buffer[0] = m_pTextureShader->m_pVertexBuffer;

	ID3D11SamplerState *sampleState = m_pTextureShader->m_pSampleState.Get();

	UpdateMatrix(pTexture->m_WorldMatrix, D3D_BASE_WIDTH, D3D_BASE_HEIGHT);
	_UpdateTextureVertexBuffer(pTexture->m_nWidth, pTexture->m_nHeight, pTexture->m_bFlipH, pTexture->m_bFlipV);
	m_pDeviceContext->PSSetSamplers(0, 1, &sampleState);
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_pDeviceContext->IASetVertexBuffers(0, 1, buffer, &stride, &offset);
	m_pDeviceContext->IASetInputLayout(m_pTextureShader->m_pInputLayout);


	/// ////////////////////////////////// 
	if (0) {
		m_pDeviceContext->PSSetShaderResources(0, count, views);
		m_pDeviceContext->Draw(TEXTURE_VERTEX_COUNT, 0);

	} else {
		D3DX11_TECHNIQUE_DESC techDesc;
		m_pTextureShader->m_pTech->GetDesc(&techDesc);

		ComPtr<ID3D11RenderTargetView> pRTV = m_WndList[m_nCurrentSwap].m_pRenderTargetView;

		ID3D11ShaderResourceView *originView = views[0];
		ID3D11Texture2D *originTexture = GetTexture(originView);

		ID3D11View *t_resource = NULL;
		ID3D11Texture2D *t_tex = NULL;
		pRTV->QueryInterface(IID_ID3D11View, (void **)&t_resource);
		t_tex = GetTexture(t_resource);

		ID3D11Texture2D *newTex = NULL;
		ID3D11ShaderResourceView *newView = NULL;
		 
		D3D11_TEXTURE2D_DESC new_tex_desc;
		originTexture->GetDesc(&new_tex_desc);

		new_tex_desc.Usage = D3D11_USAGE_DYNAMIC;
		new_tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		new_tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		new_tex_desc.MipLevels = 1;
		m_pDevice->CreateTexture2D(&new_tex_desc, NULL, &newTex);

		D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc = {};
		resourceDesc.Format = new_tex_desc.Format;
		resourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		resourceDesc.Texture2D.MipLevels = 1;
		HRESULT rs = m_pDevice->CreateShaderResourceView(newTex, &resourceDesc, &newView);

		for (UINT p = 0; p < techDesc.Passes; ++p) {
			m_pTextureShader->g_ptxDiffuse->SetResource(originView);
			m_pTextureShader->m_pTech->GetPassByIndex(p)->Apply(0, m_pDeviceContext);
			m_pDeviceContext->Draw(TEXTURE_VERTEX_COUNT, 0);
		}

		originTexture->Release();
		t_resource->Release();
		t_tex->Release();
		newTex->Release();
		newView->Release(); 
	}
}

void CRenderCenter::_SaveImage(ComPtr<ID3D11Texture2D> pSrc, const char *path)
{
	D3D11_TEXTURE2D_DESC desc = {};
	pSrc->GetDesc(&desc);

	desc.BindFlags = 0;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

	ComPtr<ID3D11Texture2D> destTexture;
	HRESULT hr = m_pDevice->CreateTexture2D(&desc, NULL, destTexture.Assign());
	if (FAILED(hr))
		return;

	m_pDeviceContext->CopyResource(destTexture, pSrc);

	D3D11_MAPPED_SUBRESOURCE map = {};
	hr = m_pDeviceContext->Map(destTexture, 0, D3D11_MAP_READ, 0, &map);
	if (FAILED(hr))
		return;

#if 1
	int wxh = desc.Width * desc.Height;

	BYTE *y0;
	BYTE *y1;
	BYTE *u0;
	BYTE *u1;
	BYTE *v0;
	BYTE *v1;
	y0 = y1 = new BYTE[wxh];
	u0 = u1 = new BYTE[wxh / 4];
	v0 = v1 = new BYTE[wxh / 4];

	int cnt = 0;
	BYTE *pData = (BYTE *)map.pData;
	for (int i = 0; i < desc.Height; i += 1) {
		BYTE *temp = pData + i * map.RowPitch;
		for (int j = 0; j < desc.Width; j += 1) {
			*y1 = temp[0];
			y1++;

			if ((i % 2) == 0 && (j % 2) == 0) {
				*u1 = temp[1];
				u1++;

				*v1 = temp[2];
				v1++;

				cnt++;
			}

			temp += 4;
		}
	}

	FILE *fp = 0;
	fopen_s(&fp, path, "wb+");
	if (fp) {
		// fwrite(&desc.Width, 4, 1, fp);
		// fwrite(&desc.Height, 4, 1, fp);
		fwrite(y0, wxh, 1, fp);
		fwrite(u0, wxh / 4, 1, fp);
		fwrite(v0, wxh / 4, 1, fp);
		fclose(fp);
	}

	delete[] y0;
	delete[] u0;
	delete[] v0;
#endif

	m_pDeviceContext->Unmap(destTexture, 0);
}

void CRenderCenter::ConvertToYUV420(const char *dir)
{
}


void SaveTextureAsFile(ID3D11Device *dev, ID3D11DeviceContext *ctx, ID3D11Texture2D *pD3DTextureSrc, char *path)
{
	D3D11_TEXTURE2D_DESC td;
	pD3DTextureSrc->GetDesc(&td);
	td.BindFlags = 0;
	td.Usage = D3D11_USAGE_STAGING;
	td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

	ID3D11Texture2D *pD3DTextureRead = NULL;
	dev->CreateTexture2D(&td, NULL, &pD3DTextureRead);
	ctx->CopyResource(pD3DTextureRead, pD3DTextureSrc);

	D3D11_MAPPED_SUBRESOURCE map;
	HRESULT hr = ctx->Map(pD3DTextureRead, 0, D3D11_MAP_READ, 0, &map);
	if (SUCCEEDED(hr)) {
		SaveBitmapFile(path, (uint8_t *)map.pData, map.RowPitch, td.Width, td.Height, 4, true);
		ctx->Unmap(pD3DTextureRead, 0);
	}

	pD3DTextureRead->Release();
}

void SaveBitmapFile(char *path, uint8_t *data, int linesize, int width, int height, int per_pixel_byte, bool flip)
{
	if (!path || !data)
		return;

	FILE *fp = NULL;
	errno_t err = fopen_s(&fp, path, "wb+");
	if (err != 0)
		return;

	unsigned dest_stride = width * per_pixel_byte;

	BITMAPFILEHEADER file_head;
	memset(&file_head, 0, sizeof(file_head));
	file_head.bfType = 'MB';
	file_head.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	file_head.bfSize = file_head.bfOffBits + height * dest_stride;

	BITMAPINFOHEADER info_head;
	memset(&info_head, 0, sizeof(info_head));
	info_head.biSize = sizeof(BITMAPINFOHEADER);
	info_head.biWidth = width;
	info_head.biHeight = height;
	info_head.biPlanes = 1;
	info_head.biBitCount = per_pixel_byte * 8;
	info_head.biCompression = 0; // 0表示无压缩
	info_head.biSizeImage = height * dest_stride;

	fwrite(&file_head, 1, sizeof(BITMAPFILEHEADER), fp);
	fwrite(&info_head, 1, sizeof(BITMAPINFOHEADER), fp);
	for (int i = 0; i < height; ++i) {
		if (flip)
			fwrite(data + (height - 1 - i) * linesize, 1, dest_stride, fp);
		else
			fwrite(data + i * linesize, 1, dest_stride, fp);
	}

	fclose(fp);
}