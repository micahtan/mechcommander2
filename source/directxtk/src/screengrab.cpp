//--------------------------------------------------------------------------------------
// File: ScreenGrab.cpp
//
// Function for capturing a 2D texture and saving it to a file (aka a 'screenshot'
// when used on a Direct3D Render Target).
//
// Note these functions are useful as a light-weight runtime screen grabber. For
// full-featured texture capture, DDS writer, and texture processing pipeline,
// see the 'Texconv' sample and the 'DirectXTex' library.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

// Does not capture 1D textures or 3D textures (volume maps)

// Does not capture mipmap chains, only the top-most texture level is saved

// For 2D array textures and cubemaps, it captures only the first image in the array

#include "stdinc.h"

#include "screengrab.h"
#include "directxhelpers.h"

#include "platformhelpers.h"
#include "dds.h"
#include "loaderhelpers.h"

using namespace DirectX;
using namespace DirectX::LoaderHelpers;

namespace
{
//--------------------------------------------------------------------------------------
HRESULT
CaptureTexture(_In_ ID3D12Device* device,
	_In_ ID3D12CommandQueue* pCommandQ,
	_In_ ID3D12Resource* pSource,
	UINT64 srcPitch,
	const D3D12_RESOURCE_DESC& desc,
	wil::com_ptr<ID3D12Resource>& pStaging,
	D3D12_RESOURCE_STATES beforeState,
	D3D12_RESOURCE_STATES afterState)
{
	if (!pCommandQ || !pSource)
		return E_INVALIDARG;

	if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
	{
		DebugTrace("ERROR: ScreenGrab does not support 1D or volume textures. Consider using DirectXTex instead.\n");
		return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
	}

	if (desc.DepthOrArraySize > 1 || desc.MipLevels > 1)
	{
		DebugTrace("WARNING: ScreenGrab does not support 2D arrays, cubemaps, or mipmaps; only the first surface is written. Consider using DirectXTex instead.\n");
	}

	if (srcPitch > UINT32_MAX)
		return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

	uint32_t numberOfPlanes = D3D12GetFormatPlaneCount(device, desc.Format);
	if (numberOfPlanes != 1)
		return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

	D3D12_HEAP_PROPERTIES sourceHeapProperties;
	D3D12_HEAP_FLAGS sourceHeapFlags;
	HRESULT hr = pSource->GetHeapProperties(&sourceHeapProperties, &sourceHeapFlags);
	if (FAILED(hr))
		return hr;

	if (sourceHeapProperties.Type == D3D12_HEAP_TYPE_READBACK)
	{
		// handle case where the source is already a staging texture we can use directly
		pStaging = pSource;
		return S_OK;
	}

	// Create a command allocator
	wil::com_ptr<ID3D12CommandAllocator> commandAlloc;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(commandAlloc.addressof()));
	if (FAILED(hr))
		return hr;

	SetDebugObjectName(commandAlloc.get(), L"ScreenGrab");

	// Spin up a new command list
	wil::com_ptr<ID3D12GraphicsCommandList> commandList;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAlloc.get(), nullptr, IID_GRAPHICS_PPV_ARGS(commandList.addressof()));
	if (FAILED(hr))
		return hr;

	SetDebugObjectName(commandList.get(), L"ScreenGrab");

	// Create a fence
	wil::com_ptr<ID3D12Fence> fence;
	hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(fence.addressof()));
	if (FAILED(hr))
		return hr;

	SetDebugObjectName(fence.get(), L"ScreenGrab");

	_ASSERT((srcPitch & 0xFF) == 0);

	CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_HEAP_PROPERTIES readBackHeapProperties(D3D12_HEAP_TYPE_READBACK);

	// Readback resources must be buffers
	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Alignment = desc.Alignment;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.flags = D3D12_RESOURCE_FLAG_NONE;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.height = 1;
	bufferDesc.width = srcPitch * desc.height;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.MipLevels = 1;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.SampleDesc.Quality = 0;

	wil::com_ptr<ID3D12Resource> copySource(pSource);
	if (desc.SampleDesc.Count > 1)
	{
		// MSAA content must be resolved before being copied to a staging texture
		auto descCopy = desc;
		descCopy.SampleDesc.Count = 1;
		descCopy.SampleDesc.Quality = 0;

		wil::com_ptr<ID3D12Resource> pTemp;
		hr = device->CreateCommittedResource(
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&descCopy,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_GRAPHICS_PPV_ARGS(pTemp.addressof()));
		if (FAILED(hr))
			return hr;

		_ASSERT(pTemp);

		SetDebugObjectName(pTemp.get(), L"ScreenGrab temporary");

		DXGI_FORMAT fmt = EnsureNotTypeless(desc.Format);

		D3D12_FEATURE_DATA_FORMAT_SUPPORT formatInfo = {fmt, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE};
		hr = device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatInfo, sizeof(formatInfo));
		if (FAILED(hr))
			return hr;

		if (!(formatInfo.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D))
			return E_FAIL;

		for (uint32_t item = 0; item < desc.DepthOrArraySize; ++item)
		{
			for (uint32_t level = 0; level < desc.MipLevels; ++level)
			{
				uint32_t index = D3D12CalcSubresource(level, item, 0, desc.MipLevels, desc.DepthOrArraySize);
				commandList->ResolveSubresource(pTemp.get(), index, pSource, index, fmt);
			}
		}

		copySource = pTemp;
	}

	// Create a staging texture
	hr = device->CreateCommittedResource(
		&readBackHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_GRAPHICS_PPV_ARGS(pStaging.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
		return hr;

	SetDebugObjectName(pStaging.get(), L"ScreenGrab staging");

	_ASSERT(pStaging);

	// Transition the resource if necessary
	TransitionResource(commandList.get(), pSource, beforeState, D3D12_RESOURCE_STATE_COPY_SOURCE);

	// Get the copy target location
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferFootprint = {};
	bufferFootprint.Footprint.width = static_cast<uint32_t>(desc.width);
	bufferFootprint.Footprint.height = desc.height;
	bufferFootprint.Footprint.Depth = 1;
	bufferFootprint.Footprint.RowPitch = static_cast<uint32_t>(srcPitch);
	bufferFootprint.Footprint.Format = desc.Format;

	CD3DX12_TEXTURE_COPY_LOCATION copydest(pStaging.get(), bufferFootprint);
	CD3DX12_TEXTURE_COPY_LOCATION copysrc(copySource.get(), 0);

	// Copy the texture
	commandList->CopyTextureRegion(&copydest, 0, 0, 0, &copysrc, nullptr);

	// Transition the resource to the next state
	TransitionResource(commandList.get(), pSource, D3D12_RESOURCE_STATE_COPY_SOURCE, afterState);

	hr = commandList->Close();
	if (FAILED(hr))
		return hr;

	// Execute the command list
	pCommandQ->ExecuteCommandLists(1, CommandListCast(commandList.addressof()));

	// Signal the fence
	hr = pCommandQ->Signal(fence.get(), 1);
	if (FAILED(hr))
		return hr;

	// Block until the copy is complete
	while (fence->GetCompletedValue() < 1)
		SwitchToThread();

	return S_OK;
}
} // anonymous namespace

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
	HRESULT
	DirectX::SaveDDSTextureToFile(
		ID3D12CommandQueue* pCommandQ,
		ID3D12Resource* pSource,
		const std::wstring_view& fileName,
		D3D12_RESOURCE_STATES beforeState,
		D3D12_RESOURCE_STATES afterState)
{
	if (!fileName)
		return E_INVALIDARG;

	wil::com_ptr<ID3D12Device> device;
	pCommandQ->GetDevice(IID_GRAPHICS_PPV_ARGS(device.addressof()));

	// Get the size of the image
	const auto desc = pSource->GetDesc();

	if (desc.width > UINT32_MAX)
		return E_INVALIDARG;

	UINT64 totalResourceSize = 0;
	UINT64 fpRowPitch = 0;
	uint32_t fpRowCount = 0;
	// Get the rowcount, pitch and size of the top mip
	device->GetCopyableFootprints(
		&desc,
		0,
		1,
		0,
		nullptr,
		&fpRowCount,
		&fpRowPitch,
		&totalResourceSize);

#if defined(_XBOX_ONE) && defined(_TITLE)
	// Round up the srcPitch to multiples of 1024
	UINT64 dstRowPitch = (fpRowPitch + static_cast<uint64_t>(D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT) - 1u) & ~(static_cast<uint64_t>(D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT) - 1u);
#else
	// Round up the srcPitch to multiples of 256
	UINT64 dstRowPitch = (fpRowPitch + 255) & ~0xFFu;
#endif

	if (dstRowPitch > UINT32_MAX)
		return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

	wil::com_ptr<ID3D12Resource> pStaging;
	HRESULT hr = CaptureTexture(device.get(), pCommandQ, pSource, dstRowPitch, desc, pStaging, beforeState, afterState);
	if (FAILED(hr))
		return hr;

	// Create file
	ScopedHandle hFile(safe_handle(CreateFile2(fileName, GENERIC_WRITE, 0, CREATE_ALWAYS, nullptr)));
	if (!hFile)
		return HRESULT_FROM_WIN32(GetLastError());

	auto_delete_file delonfail(hFile.get());

	// Setup header
	const size_t MAX_HEADER_SIZE = sizeof(uint32_t) + sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10);
	uint8_t fileHeader[MAX_HEADER_SIZE];

	*reinterpret_cast<uint32_t*>(&fileHeader[0]) = DDS_MAGIC;

	auto header = reinterpret_cast<DDS_HEADER*>(&fileHeader[0] + sizeof(uint32_t));
	size_t headerSize = sizeof(uint32_t) + sizeof(DDS_HEADER);
	memset(header, 0, sizeof(DDS_HEADER));
	header->size = sizeof(DDS_HEADER);
	header->flags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_MIPMAP;
	header->height = desc.height;
	header->width = static_cast<uint32_t>(desc.width);
	header->mipMapCount = 1;
	header->caps = DDS_SURFACE_FLAGS_TEXTURE;

	// Try to use a legacy .DDS pixel format for better tools support, otherwise fallback to 'DX10' header extension
	DDS_HEADER_DXT10* extHeader = nullptr;
	switch (desc.Format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_A8B8G8R8, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_R16G16_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_G16R16, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_R8G8_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_A8L8, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_R16_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_L16, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_R8_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_L8, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_A8_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_A8, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_R8G8_B8G8, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_G8R8_G8B8, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_BC1_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_DXT1, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_BC2_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_DXT3, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_BC3_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_DXT5, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_BC4_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_BC4_UNORM, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_BC4_SNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_BC4_SNORM, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_BC5_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_BC5_UNORM, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_BC5_SNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_BC5_SNORM, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_B5G6R5_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_R5G6B5, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_B5G5R5A1_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_A1R5G5B5, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_R8G8_SNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_V8U8, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_R8G8B8A8_SNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_Q8W8V8U8, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_R16G16_SNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_V16U16, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_A8R8G8B8, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_X8R8G8B8, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_YUY2:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_YUY2, sizeof(DDS_PIXELFORMAT));
		break;
	case DXGI_FORMAT_B4G4R4A4_UNORM:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_A4R4G4B4, sizeof(DDS_PIXELFORMAT));
		break;

		// Legacy D3DX formats using D3DFMT enum value as FourCC
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		header->ddspf.size = sizeof(DDS_PIXELFORMAT);
		header->ddspf.flags = DDS_FOURCC;
		header->ddspf.fourCC = 116;
		break; // D3DFMT_A32B32G32R32F
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		header->ddspf.size = sizeof(DDS_PIXELFORMAT);
		header->ddspf.flags = DDS_FOURCC;
		header->ddspf.fourCC = 113;
		break; // D3DFMT_A16B16G16R16F
	case DXGI_FORMAT_R16G16B16A16_UNORM:
		header->ddspf.size = sizeof(DDS_PIXELFORMAT);
		header->ddspf.flags = DDS_FOURCC;
		header->ddspf.fourCC = 36;
		break; // D3DFMT_A16B16G16R16
	case DXGI_FORMAT_R16G16B16A16_SNORM:
		header->ddspf.size = sizeof(DDS_PIXELFORMAT);
		header->ddspf.flags = DDS_FOURCC;
		header->ddspf.fourCC = 110;
		break; // D3DFMT_Q16W16V16U16
	case DXGI_FORMAT_R32G32_FLOAT:
		header->ddspf.size = sizeof(DDS_PIXELFORMAT);
		header->ddspf.flags = DDS_FOURCC;
		header->ddspf.fourCC = 115;
		break; // D3DFMT_G32R32F
	case DXGI_FORMAT_R16G16_FLOAT:
		header->ddspf.size = sizeof(DDS_PIXELFORMAT);
		header->ddspf.flags = DDS_FOURCC;
		header->ddspf.fourCC = 112;
		break; // D3DFMT_G16R16F
	case DXGI_FORMAT_R32_FLOAT:
		header->ddspf.size = sizeof(DDS_PIXELFORMAT);
		header->ddspf.flags = DDS_FOURCC;
		header->ddspf.fourCC = 114;
		break; // D3DFMT_R32F
	case DXGI_FORMAT_R16_FLOAT:
		header->ddspf.size = sizeof(DDS_PIXELFORMAT);
		header->ddspf.flags = DDS_FOURCC;
		header->ddspf.fourCC = 111;
		break; // D3DFMT_R16F

	case DXGI_FORMAT_AI44:
	case DXGI_FORMAT_IA44:
	case DXGI_FORMAT_P8:
	case DXGI_FORMAT_A8P8:
		DebugTrace("ERROR: ScreenGrab does not support video textures. Consider using DirectXTex.\n");
		return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

	default:
		memcpy_s(&header->ddspf, sizeof(header->ddspf), &DDSPF_DX10, sizeof(DDS_PIXELFORMAT));

		headerSize += sizeof(DDS_HEADER_DXT10);
		extHeader = reinterpret_cast<DDS_HEADER_DXT10*>(fileHeader + sizeof(uint32_t) + sizeof(DDS_HEADER));
		memset(extHeader, 0, sizeof(DDS_HEADER_DXT10));
		extHeader->dxgiFormat = desc.Format;
		extHeader->resourceDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		extHeader->arraySize = 1;
		break;
	}

	size_t rowPitch, slicePitch, rowCount;
	hr = GetSurfaceInfo(static_cast<size_t>(desc.width), desc.height, desc.Format, &slicePitch, &rowPitch, &rowCount);
	if (FAILED(hr))
		return hr;

	if (rowPitch > UINT32_MAX || slicePitch > UINT32_MAX)
		return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

	if (IsCompressed(desc.Format))
	{
		header->flags |= DDS_HEADER_FLAGS_LINEARSIZE;
		header->pitchOrLinearSize = static_cast<uint32_t>(slicePitch);
	}
	else
	{
		header->flags |= DDS_HEADER_FLAGS_PITCH;
		header->pitchOrLinearSize = static_cast<uint32_t>(rowPitch);
	}

	// Setup pixels
	std::unique_ptr<uint8_t[]> pixels(new (std::nothrow) uint8_t[slicePitch]);
	if (!pixels)
		return E_OUTOFMEMORY;

	_ASSERT(fpRowCount == rowCount);
	_ASSERT(fpRowPitch == rowPitch);

	UINT64 imageSize = dstRowPitch * UINT64(rowCount);
	if (imageSize > UINT32_MAX)
		return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

	void* pMappedMemory = nullptr;
	D3D12_RANGE readRange = {0, static_cast<SIZE_T>(imageSize)};
	D3D12_RANGE writeRange = {0, 0};
	hr = pStaging->Map(0, &readRange, &pMappedMemory);
	if (FAILED(hr))
		return hr;

	auto sptr = static_cast<const uint8_t*>(pMappedMemory);
	if (!sptr)
	{
		pStaging->Unmap(0, &writeRange);
		return E_POINTER;
	}

	uint8_t* dptr = pixels.get();

	size_t msize = std::min<size_t>(rowPitch, rowPitch);
	for (size_t h = 0; h < rowCount; ++h)
	{
		memcpy_s(dptr, rowPitch, sptr, msize);
		sptr += dstRowPitch;
		dptr += rowPitch;
	}

	pStaging->Unmap(0, &writeRange);

	// Write header & pixels
	uint32_t bytesWritten;
	if (!WriteFile(hFile.get(), fileHeader, static_cast<uint32_t>(headerSize), &bytesWritten, nullptr))
		return HRESULT_FROM_WIN32(GetLastError());

	if (bytesWritten != headerSize)
		return E_FAIL;

	if (!WriteFile(hFile.get(), pixels.get(), static_cast<uint32_t>(slicePitch), &bytesWritten, nullptr))
		return HRESULT_FROM_WIN32(GetLastError());

	if (bytesWritten != slicePitch)
		return E_FAIL;

	delonfail.clear();

	return S_OK;
}

//--------------------------------------------------------------------------------------
namespace DirectX
{
extern IWICImagingFactory2*
_GetWIC();
}

_Use_decl_annotations_
	HRESULT
	DirectX::SaveWICTextureToFile(
		ID3D12CommandQueue* pCommandQ,
		ID3D12Resource* pSource,
		REFGUID guidContainerFormat,
		const std::wstring_view& fileName,
		D3D12_RESOURCE_STATES beforeState,
		D3D12_RESOURCE_STATES afterState,
		const GUID* targetFormat,
		std::function<void(IPropertyBag2*)> setCustomProps)
{
	if (!fileName)
		return E_INVALIDARG;

	wil::com_ptr<ID3D12Device> device;
	pCommandQ->GetDevice(IID_GRAPHICS_PPV_ARGS(device.addressof()));

	// Get the size of the image
	const auto desc = pSource->GetDesc();

	if (desc.width > UINT32_MAX)
		return E_INVALIDARG;

	UINT64 totalResourceSize = 0;
	UINT64 fpRowPitch = 0;
	uint32_t fpRowCount = 0;
	// Get the rowcount, pitch and size of the top mip
	device->GetCopyableFootprints(
		&desc,
		0,
		1,
		0,
		nullptr,
		&fpRowCount,
		&fpRowPitch,
		&totalResourceSize);

#if defined(_XBOX_ONE) && defined(_TITLE)
	// Round up the srcPitch to multiples of 1024
	UINT64 dstRowPitch = (fpRowPitch + static_cast<uint64_t>(D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT) - 1u) & ~(static_cast<uint64_t>(D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT) - 1u);
#else
	// Round up the srcPitch to multiples of 256
	UINT64 dstRowPitch = (fpRowPitch + 255) & ~0xFFu;
#endif

	if (dstRowPitch > UINT32_MAX)
		return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

	wil::com_ptr<ID3D12Resource> pStaging;
	HRESULT hr = CaptureTexture(device.get(), pCommandQ, pSource, dstRowPitch, desc, pStaging, beforeState, afterState);
	if (FAILED(hr))
		return hr;

	// Determine source format's WIC equivalent
	WICPixelFormatGUID pfGuid;
	bool sRGB = false;
	switch (desc.Format)
	{
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		pfGuid = GUID_WICPixelFormat128bppRGBAFloat;
		break;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		pfGuid = GUID_WICPixelFormat64bppRGBAHalf;
		break;
	case DXGI_FORMAT_R16G16B16A16_UNORM:
		pfGuid = GUID_WICPixelFormat64bppRGBA;
		break;
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		pfGuid = GUID_WICPixelFormat32bppRGBA1010102XR;
		break;
	case DXGI_FORMAT_R10G10B10A2_UNORM:
		pfGuid = GUID_WICPixelFormat32bppRGBA1010102;
		break;
	case DXGI_FORMAT_B5G5R5A1_UNORM:
		pfGuid = GUID_WICPixelFormat16bppBGRA5551;
		break;
	case DXGI_FORMAT_B5G6R5_UNORM:
		pfGuid = GUID_WICPixelFormat16bppBGR565;
		break;
	case DXGI_FORMAT_R32_FLOAT:
		pfGuid = GUID_WICPixelFormat32bppGrayFloat;
		break;
	case DXGI_FORMAT_R16_FLOAT:
		pfGuid = GUID_WICPixelFormat16bppGrayHalf;
		break;
	case DXGI_FORMAT_R16_UNORM:
		pfGuid = GUID_WICPixelFormat16bppGray;
		break;
	case DXGI_FORMAT_R8_UNORM:
		pfGuid = GUID_WICPixelFormat8bppGray;
		break;
	case DXGI_FORMAT_A8_UNORM:
		pfGuid = GUID_WICPixelFormat8bppAlpha;
		break;

	case DXGI_FORMAT_R8G8B8A8_UNORM:
		pfGuid = GUID_WICPixelFormat32bppRGBA;
		break;

	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		pfGuid = GUID_WICPixelFormat32bppRGBA;
		sRGB = true;
		break;

	case DXGI_FORMAT_B8G8R8A8_UNORM:
		pfGuid = GUID_WICPixelFormat32bppBGRA;
		break;

	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		pfGuid = GUID_WICPixelFormat32bppBGRA;
		sRGB = true;
		break;

	case DXGI_FORMAT_B8G8R8X8_UNORM:
		pfGuid = GUID_WICPixelFormat32bppBGR;
		break;

	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		pfGuid = GUID_WICPixelFormat32bppBGR;
		sRGB = true;
		break;

	default:
		DebugTrace("ERROR: ScreenGrab does not support all DXGI formats (%u). Consider using DirectXTex.\n", static_cast<uint32_t>(desc.Format));
		return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
	}

	auto pWIC = _GetWIC();
	if (!pWIC)
		return E_NOINTERFACE;

	wil::com_ptr<IWICStream> stream;
	hr = pWIC->CreateStream(stream.addressof());
	if (FAILED(hr))
		return hr;

	hr = stream->InitializeFromFilename(fileName, GENERIC_WRITE);
	if (FAILED(hr))
		return hr;

	auto_delete_file_wic delonfail(stream, fileName);

	wil::com_ptr<IWICBitmapEncoder> encoder;
	hr = pWIC->CreateEncoder(guidContainerFormat, nullptr, encoder.addressof());
	if (FAILED(hr))
		return hr;

	hr = encoder->Initialize(stream.get(), WICBitmapEncoderNoCache);
	if (FAILED(hr))
		return hr;

	wil::com_ptr<IWICBitmapFrameEncode> frame;
	wil::com_ptr<IPropertyBag2> props;
	hr = encoder->CreateNewFrame(frame.addressof(), props.addressof());
	if (FAILED(hr))
		return hr;

	if (targetFormat && memcmp(&guidContainerFormat, &GUID_ContainerFormatBmp, sizeof(WICPixelFormatGUID)) == 0)
	{
		// Opt-in to the WIC2 support for writing 32-bit Windows BMP files with an alpha channel
		PROPBAG2 option = {};
		option.pstrName = const_cast<wchar_t*>(L"EnableV5Header32bppBGRA");

		VARIANT varValue;
		varValue.vt = VT_BOOL;
		varValue.boolVal = VARIANT_TRUE;
		(void)props->Write(1, &option, &varValue);
	}

	if (setCustomProps)
	{
		setCustomProps(props.get());
	}

	hr = frame->Initialize(props.get());
	if (FAILED(hr))
		return hr;

	hr = frame->SetSize(static_cast<uint32_t>(desc.width), desc.height);
	if (FAILED(hr))
		return hr;

	hr = frame->SetResolution(72, 72);
	if (FAILED(hr))
		return hr;

	// Pick a target format
	WICPixelFormatGUID targetGuid;
	if (targetFormat)
	{
		targetGuid = *targetFormat;
	}
	else
	{
		// Screenshots don't typically include the alpha channel of the render target
		switch (desc.Format)
		{
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			targetGuid = GUID_WICPixelFormat96bppRGBFloat; // WIC 2
			break;

		case DXGI_FORMAT_R16G16B16A16_UNORM:
			targetGuid = GUID_WICPixelFormat48bppBGR;
			break;
		case DXGI_FORMAT_B5G5R5A1_UNORM:
			targetGuid = GUID_WICPixelFormat16bppBGR555;
			break;
		case DXGI_FORMAT_B5G6R5_UNORM:
			targetGuid = GUID_WICPixelFormat16bppBGR565;
			break;

		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_A8_UNORM:
			targetGuid = GUID_WICPixelFormat8bppGray;
			break;

		default:
			targetGuid = GUID_WICPixelFormat24bppBGR;
			break;
		}
	}

	hr = frame->SetPixelFormat(&targetGuid);
	if (FAILED(hr))
		return hr;

	if (targetFormat && memcmp(targetFormat, &targetGuid, sizeof(WICPixelFormatGUID)) != 0)
	{
		// Requested output pixel format is not supported by the WIC codec
		return E_FAIL;
	}

	// Encode WIC metadata
	wil::com_ptr<IWICMetadataQueryWriter> metawriter;
	if (SUCCEEDED(frame->GetMetadataQueryWriter(metawriter.addressof())))
	{
		PROPVARIANT value;
		PropVariantInit(&value);

		value.vt = VT_LPSTR;
		value.pszVal = const_cast<wchar_t*>("DirectXTK");

		if (memcmp(&guidContainerFormat, &GUID_ContainerFormatPng, sizeof(GUID)) == 0)
		{
			// Set Software name
			(void)metawriter->SetMetadataByName(L"/tEXt/{str=Software}", &value);

			// Set sRGB chunk
			if (sRGB)
			{
				value.vt = VT_UI1;
				value.bVal = 0;
				(void)metawriter->SetMetadataByName(L"/sRGB/RenderingIntent", &value);
			}
			else
			{
				// add gAMA chunk with gamma 1.0
				value.vt = VT_UI4;
				value.uintVal = 100000; // gama value * 100,000 -- i.e. gamma 1.0
				(void)metawriter->SetMetadataByName(L"/gAMA/ImageGamma", &value);

				// remove sRGB chunk which is added by default.
				(void)metawriter->RemoveMetadataByName(L"/sRGB/RenderingIntent");
			}
		}
#if defined(_XBOX_ONE) && defined(_TITLE)
		else if (memcmp(&guidContainerFormat, &GUID_ContainerFormatJpeg, sizeof(GUID)) == 0)
		{
			// Set Software name
			(void)metawriter->SetMetadataByName(L"/app1/ifd/{ushort=305}", &value);

			if (sRGB)
			{
				// Set EXIF colourspace of sRGB
				value.vt = VT_UI2;
				value.uiVal = 1;
				(void)metawriter->SetMetadataByName(L"/app1/ifd/exif/{ushort=40961}", &value);
			}
		}
		else if (memcmp(&guidContainerFormat, &GUID_ContainerFormatTiff, sizeof(GUID)) == 0)
		{
			// Set Software name
			(void)metawriter->SetMetadataByName(L"/ifd/{ushort=305}", &value);

			if (sRGB)
			{
				// Set EXIF colourspace of sRGB
				value.vt = VT_UI2;
				value.uiVal = 1;
				(void)metawriter->SetMetadataByName(L"/ifd/exif/{ushort=40961}", &value);
			}
		}
#else
		else
		{
			// Set Software name
			(void)metawriter->SetMetadataByName(L"System.ApplicationName", &value);

			if (sRGB)
			{
				// Set EXIF colourspace of sRGB
				value.vt = VT_UI2;
				value.uiVal = 1;
				(void)metawriter->SetMetadataByName(L"System.Image.colourSpace", &value);
			}
		}
#endif
	}

	UINT64 imageSize = dstRowPitch * UINT64(desc.height);
	if (imageSize > UINT32_MAX)
		return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

	void* pMappedMemory = nullptr;
	D3D12_RANGE readRange = {0, static_cast<SIZE_T>(imageSize)};
	D3D12_RANGE writeRange = {0, 0};
	hr = pStaging->Map(0, &readRange, &pMappedMemory);
	if (FAILED(hr))
		return hr;

	if (memcmp(&targetGuid, &pfGuid, sizeof(WICPixelFormatGUID)) != 0)
	{
		// Conversion required to write
		wil::com_ptr<IWICBitmap> source;
		hr = pWIC->CreateBitmapFromMemory(static_cast<uint32_t>(desc.width), desc.height, pfGuid,
			static_cast<uint32_t>(dstRowPitch), static_cast<uint32_t>(imageSize),
			static_cast<BYTE*>(pMappedMemory), source.addressof());
		if (FAILED(hr))
		{
			pStaging->Unmap(0, &writeRange);
			return hr;
		}

		wil::com_ptr<IWICFormatConverter> FC;
		hr = pWIC->CreateFormatConverter(FC.addressof());
		if (FAILED(hr))
		{
			pStaging->Unmap(0, &writeRange);
			return hr;
		}

		BOOL canConvert = FALSE;
		hr = FC->CanConvert(pfGuid, targetGuid, &canConvert);
		if (FAILED(hr) || !canConvert)
		{
			return E_UNEXPECTED;
		}

		hr = FC->Initialize(source.get(), targetGuid, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeMedianCut);
		if (FAILED(hr))
		{
			pStaging->Unmap(0, &writeRange);
			return hr;
		}

		WICRect rect = {0, 0, static_cast<INT>(desc.width), static_cast<INT>(desc.height)};
		hr = frame->WriteSource(FC.get(), &rect);
		if (FAILED(hr))
		{
			pStaging->Unmap(0, &writeRange);
			return hr;
		}
	}
	else
	{
		// No conversion required
		hr = frame->WritePixels(desc.height, static_cast<uint32_t>(dstRowPitch), static_cast<uint32_t>(imageSize), static_cast<BYTE*>(pMappedMemory));
		if (FAILED(hr))
			return hr;
	}

	pStaging->Unmap(0, &writeRange);

	hr = frame->Commit();
	if (FAILED(hr))
		return hr;

	hr = encoder->Commit();
	if (FAILED(hr))
		return hr;

	delonfail.clear();

	return S_OK;
}
