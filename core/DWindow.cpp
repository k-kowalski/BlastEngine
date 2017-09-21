#include "stdafx.h"
#include "DWindow.h"

using namespace DirectX;
using namespace Microsoft::WRL;

DWindow::DWindow(
	int cliWidth,
	int cliHeight,
	int xloc,
	int yloc,
	LPCTSTR szWindowName,
	DWORD dwStyle) : BaseWindow(cliWidth, cliHeight, xloc, yloc, szWindowName, dwStyle)
{
	//initialize graphics pipeline and load assets, then comm loop takes care of issuing 'paint' messages
	LoadPipeline();
	LoadAssets();

	ShowWindow(1);
}


DWindow::~DWindow()
{
}

void DWindow::initWorld()
{
	// build projection and view matrix
	XMMATRIX tmpMat = XMMatrixPerspectiveFovLH(45.0f*(3.14f / 180.0f), (float)getClientWidth() / (float)getClientHeight(), 0.1f, 1000.0f);
	XMStoreFloat4x4(&cameraProjMat, tmpMat);

	// set starting camera state
	cameraPosition = XMFLOAT4(0.4f, 2.0f, -4.0f, 0.0f);
	cameraTarget = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	cameraUp = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

	// build view matrix
	XMVECTOR cPos = XMLoadFloat4(&cameraPosition);
	XMVECTOR cTarg = XMLoadFloat4(&cameraTarget);
	XMVECTOR cUp = XMLoadFloat4(&cameraUp);
	tmpMat = XMMatrixLookAtLH(cPos, cTarg, cUp);
	XMStoreFloat4x4(&cameraViewMat, tmpMat);

	// set starting cubes position
	// first cube
	cube1Position = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // set cube 1's position
	XMVECTOR posVec = XMLoadFloat4(&cube1Position); // create xmvector for cube1's position

	tmpMat = XMMatrixTranslationFromVector(posVec); // create translation matrix from cube1's position vector
	XMStoreFloat4x4(&cube1RotMat, XMMatrixIdentity()); // initialize cube1's rotation matrix to identity matrix
	XMStoreFloat4x4(&cube1WorldMat, tmpMat); // store cube1's world matrix

}

void DWindow::updateWorld()
{
	// update app logic, such as moving the camera or figuring out what objects are in view

	// create rotation matrices
	XMMATRIX rotXMat = XMMatrixRotationX(0.008f);
	XMMATRIX rotYMat = XMMatrixRotationY(0.009f);
	XMMATRIX rotZMat = XMMatrixRotationZ(0.01f);

	// add rotation to cube1's rotation matrix and store it
	XMMATRIX rotMat = XMLoadFloat4x4(&cube1RotMat) * rotXMat * rotYMat * rotZMat;
	XMStoreFloat4x4(&cube1RotMat, rotMat);

	// create translation matrix for cube 1 from cube 1's position vector
	XMMATRIX translationMat = XMMatrixTranslationFromVector(XMLoadFloat4(&cube1Position));

	// create cube1's world matrix by first rotating the cube, then positioning the rotated cube
	XMMATRIX worldMat = rotMat * translationMat;

	// store cube1's world matrix
	XMStoreFloat4x4(&cube1WorldMat, worldMat);

	// update constant buffer for cube1
	// create the wvp matrix and store in constant buffer
	XMMATRIX viewMat = XMLoadFloat4x4(&cameraViewMat); // load view matrix
	XMMATRIX projMat = XMLoadFloat4x4(&cameraProjMat); // load projection matrix
	XMMATRIX wvpMat = XMLoadFloat4x4(&cube1WorldMat) * viewMat * projMat; // create wvp matrix
	XMMATRIX transposed = XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
	XMStoreFloat4x4(&m_newConstantBufferData.mat, transposed); // store transposed wvp matrix in constant buffer

	memcpy(m_pCbvDataBegin, &m_newConstantBufferData, sizeof(m_newConstantBufferData));
}

void DWindow::LoadPipeline()
{

	int he = getClientHeight();
	int wi = getClientWidth();
	m_aspectRatio = static_cast<float>(wi) / static_cast<float>(he);

	m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(wi), static_cast<float>(he));
	m_scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(wi), static_cast<LONG>(he));

	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)

	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));



	ComPtr<IDXGIAdapter> warpAdapter;
	factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));

	D3D12CreateDevice(
		warpAdapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_device)
	);


	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));



	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = wi;
	swapChainDesc.Height = he;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
		hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	);

	// This sample does not support fullscreen transitions.
	factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

	swapChain.As(&m_swapChain);
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Describe and create a constant buffer view (CBV) descriptor heap.
		// Flags indicate that this descriptor heap can be bound to the pipeline 
		// and that descriptors contained in it can be referenced by a root table.
		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
		cbvHeapDesc.NumDescriptors = 1;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap));
	}

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV and a command allocator for each frame.
		for (UINT n = 0; n < FrameCount; n++)
		{
			m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);

			m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[n]));
		}
	}


	m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&m_bundleAllocator));
}

void DWindow::LoadAssets()
{
	// Create a root signature consisting of a descriptor table with a single CBV.
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

		// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
		CD3DX12_ROOT_PARAMETER1 rootParameters[1];

		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

		// Allow input layout and deny uneccessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

		ComPtr<ID3DBlob> signature;
		D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, 0);
		m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
	}

	
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

		{		
			// for now we will compile at runtime, so no need to include to .proj
			UINT compileFlags;
#if defined(_DEBUG)
			// shader debugging
			compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			compileFlags = 0;
#endif

			D3DCompileFromFile(L"PixVert.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr);
			D3DCompileFromFile(L"PixVert.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr);
		}

		

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // a default depth stencil state
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;

		m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
	}

	// Create the command list.
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList));

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	m_commandList->Close();

	// Create the vertex buffer.
	{
		//Vertex verts[] = {
		//	{ { -0.1f,  0.1f, 0.1f },{ 1.0f, 0.0f, 0.0f, 1.0f } },
		//	{ { 0.1f, -0.1f, 0.1f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
		//	{ { -0.1f, -0.1f, 0.1f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		//	{ { 0.1f,  0.1f, 0.1f },{ 1.0f, 0.0f, 1.0f, 1.0f } },
		//	// second quad (further from camera, green)
		//	{ { -0.2f,  0.2f,  0.2f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
		//	{ { 0.0f,  0.0f, 0.2f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
		//	{ { -0.2f,  0.0f, 0.2f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
		//	{ { 0.0f,  0.2f,  0.2f },{ 0.0f, 1.0f, 0.0f, 1.0f } }
		//};
		//maybe in std::vector? array for now
		Vertex verts[] =
		{
			// front face
			{ -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
			{ 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
			{ -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
			{ 0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

			// right side face
			{ 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
			{ 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
			{ 0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
			{ 0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

			// left side face
			{ -0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
			{ -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
			{ -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
			{ -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

			// back face
			{ 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
			{ -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
			{ 0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
			{ -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

			// top face
			{ -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
			{ 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
			{ 0.5f,  0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
			{ -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

			// bottom face
			{ 0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
			{ -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
			{ 0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
			{ -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
		};

		const UINT vertexBufferSize = sizeof(verts);

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer));

		// Copy the data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		//poll space
		m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
		//copy to specified place
		memcpy(pVertexDataBegin, verts, vertexBufferSize);
		m_vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;

	}

	//create the index buffer
	{
		/*DWORD indices[] = {
			0, 1, 2,
			0, 3, 1
		};*/

		DWORD indices[] =
		{
			0, 1, 2, // first triangle
			0, 3, 1, // second triangle

					 // left face
					 4, 5, 6, // first triangle
					 4, 7, 5, // second triangle

							  // right face
							  8, 9, 10, // first triangle
							  8, 11, 9, // second triangle

										// back face
										12, 13, 14, // first triangle
										12, 15, 13, // second triangle

													// top face
													16, 17, 18, // first triangle
													16, 19, 17, // second triangle

																// bottom face
																20, 21, 22, // first triangle
																20, 23, 21, // second triangle
		};

		//how many indices are we dealing with
		indexCount = sizeof(indices) / sizeof(DWORD);

		const UINT indexBufferSize = sizeof(indices);

		//create upload heap for indices
		m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_indexBuffer));

		UINT8* pIndexDataBegin;
		//poll space
		CD3DX12_RANGE readRange(0, 0);
		m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
		//copy to specified place
		memcpy(pIndexDataBegin, indices, indexBufferSize);

		// Initialize the vertex buffer view.
		m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
		m_indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 32-bit unsigned integer (this is what a DWORD is, double word, a word is 2 bytes)
		m_indexBufferView.SizeInBytes = indexBufferSize;
	}


	{
		// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsDescriptorHeap));

		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

		D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
		depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil = 0;

		m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, getClientWidth(), getClientHeight(), 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthOptimizedClearValue,
			IID_PPV_ARGS(&depthStencilBuffer)
		);

		m_device->CreateDepthStencilView(depthStencilBuffer.Get(), &depthStencilDesc, dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// Create and record the bundle.
	{
		m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, m_bundleAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_bundle));

		m_bundle->SetGraphicsRootSignature(m_rootSignature.Get());
		m_bundle->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_bundle->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		m_bundle->IASetIndexBuffer(&m_indexBufferView);

		m_bundle->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
	
		m_bundle->Close();
	}

	// Create the constant buffer.
	{
		m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_newConstantBuffer));

		// Describe and create a constant buffer view.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_newConstantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = (sizeof(NewConstantBuffer) + 255) & ~255;	// CB size is required to be 256-byte aligned.
		m_device->CreateConstantBufferView(&cbvDesc, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());

		// Map and initialize the constant buffer. We don't unmap this until the
		// app closes. Keeping things mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		m_newConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin));
		memcpy(m_pCbvDataBegin, &m_newConstantBufferData, sizeof(m_newConstantBufferData));
	}

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
		m_fenceValues[m_frameIndex]++;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		initWorld();

		// Wait for the command list to execute; we are reusing the same command 
		// list in our main loop but for now, we just want to wait for setup to 
		// complete before continuing.
		WaitForGPU();
	}
}

// Fill the command list with all the render commands and dependent state.
void DWindow::PopulateCommandList()
{
	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	m_commandAllocators[m_frameIndex]->Reset();

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get());

	// Set necessary state.
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { m_cbvHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Indicate that the back buffer will be used as a render target.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);

	// get a handle to the depth/stencil buffer
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// set the render target for the output merger stage (the output of the pipeline)
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	// clear the depth/stencil buffer
	m_commandList->ClearDepthStencilView(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	// Execute the commands stored in the bundle.
	m_commandList->ExecuteBundle(m_bundle.Get());

	// Indicate that the back buffer will now be used to present.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	m_commandList->Close();
}

void DWindow::MoveToNextFrame()
{
	// Schedule a Signal command in the queue.
	const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
	m_commandQueue->Signal(m_fence.Get(), currentFenceValue);

	// Update the frame index.
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// If the next frame is not ready to be rendered yet, wait until it is ready.
	if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
	{
		m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}

	// Set the fence value for the next frame.
	m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}

void DWindow::WaitForGPU()
{
	// Schedule a Signal command in the queue.
	m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]);

	// Wait until the fence has been processed.
	m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

	// Increment the fence value for the current frame.
	m_fenceValues[m_frameIndex]++;
}

//nothing?
LRESULT DWindow::OnKeyDown(UINT, WPARAM wparam, LPARAM, BOOL &)
{

	switch (wparam)
	{
	case VK_RIGHT:
	
		break;

	case VK_LEFT:
	

		break;

	case VK_UP:
	

		break;

	case VK_DOWN:
	

		break;

	case VK_ESCAPE:
		DestroyWindow();
		break;

	default:
		break;
	}

	return 0;
}


LRESULT DWindow::OnPaint(UINT, WPARAM, LPARAM, BOOL &)
{
	//run logic update
	updateWorld();

	// Record all the commands we need to render the scene into the command list.
	PopulateCommandList();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	m_swapChain->Present(1, 0);

	MoveToNextFrame();

	return 0;
}

LRESULT DWindow::OnDestroy(UINT, WPARAM, LPARAM, BOOL &)
{
	WaitForGPU();
	CloseHandle(m_fenceEvent);

	PostQuitMessage(0);
	return 0;
}

//nothing?
LRESULT DWindow::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{

	return 0;
}

//nothing?
LRESULT DWindow::OnKeyUp(UINT /*uMsg*/, WPARAM wparam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	switch (wparam)
	{
	case VK_RIGHT:
		
		break;

	case VK_LEFT:
		
		break;

	case VK_UP:
		
		break;

	case VK_DOWN:
		
		break;

	case VK_ESCAPE:
		
		break;

	default:

		break;
	}

	return 0;
}