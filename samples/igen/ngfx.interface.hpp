#include "ngfx.structs.hpp"

namespace ngfx {
    interface Blob {
        const void* Data() const;
        uint64      Length() const;
    };
    interface Function {
        const char* EntryPoint();
        const Blob* Bundle();
    };
    struct RenderPipelineDesc {
        RasterizerState     rasterizer;
        BlendState          blend;
        DepthStencilState   depthStencil;
        VertexInputState    input;

        PixelFormat         depthStencilFormat;
        array<Function>     shaders;
        uint32              deviceMask;
    };
    struct ComputePipelineDesc {

    };
    struct RaytracingPipelineDesc {

    };
	struct RenderpassDesc {

	};
    interface Resource {
		void*			Map(uint64 offset, uint64 size);
		void			Unmap(void* addr);
    };
	[[vulkan("VkImage"), metal("id<MTLTexture>")]]
    interface Texture : Resource {
		PixelFormat Format() const;
    };
	interface TextureView {
		const Texture*	GetTexture() const;
		TextureUsage	GetUsage() const;
	};
	[[vulkan("VkBuffer"), metal("id<MTLBuffer>")]]
    interface Buffer : Resource {
    };
	interface BufferView {
		const Buffer*	GetBuffer() const;
		BufferUsage		GetUsage() const;
	};
	interface RaytracingAS {

	};
	[[vulkan("VkSampler"), metal("id<MTLSampler>")]]
    interface Sampler {

    };

    interface Shader {

    };
	[[vulkan("VkFramebuffer")]]
    interface Framebuffer {

    };
	[[vulkan("VkRenderpass")]]
    interface Renderpass {

    };
	[[vulkan("VkSwapchain")]]
    interface Swapchain {
		Texture* CurrentTexture();
    };
	[[transient("frame")]]
	interface BindGroup {
		void SetSampler(uint32 id, ShaderStage stage, const Sampler* texture);
		void SetTexture(uint32 id, ShaderStage stage, const TextureView* texture);
		void SetBuffer(uint32 id, ShaderStage stage, const BufferView* texture);
	};
    interface Pipeline {
		BindGroup*  NewBindGroup(Result * result);
		Device*		GetDevice() const;
    };

    interface RenderPipeline : Pipeline {

    };
    
    interface ComputePipeline : Pipeline {

    };

    interface RaytracePipeline : Pipeline {

    };
	[[vulkan("VkCommandBuffer"), metal("id<MTLCommandBuffer>")]]
    interface CommandBuffer {
        Result NewRenderEncoder();
        Result NewComputeEncoder();
		Result NewBlitEncoder();
        Result NewParallelRenderEncoder();
        Result NewRaytracingEncoder();
        Result Submit();
    };
	[[vulkan("VkQueue"), metal("id<MTLQueue>")]]
    interface CommandQueue {
		CommandBuffer* NewCommandBuffer() [[transient("true")]];
    };

    interface CommandEncoder {
        void EndEncode();
    };

    interface RenderEncoder {
        void SetRenderPipepline(const RenderPipeline * render);
		void BindGroup(const BindGroup* bind);
        void Draw();
        void EndRenderpass();
        void Present(Swapchain* swapchain);
    };

    interface ComputeEncoder {
        void Dispatch(int x, int y, int z);
    };

    interface RaytraceEncoder {
        void TraceRay(int width, int height);
    };
	[[vulkan("VkFence"), metal("id<MTLFence>")]]
    interface Fence {
        void Signal();
    };
	[[vulkan("VkDevice"), metal("id<MTLDevice>")]]
    interface Device {
		CommandQueue*	NewQueue();
        Shader*			NewShader();
        Renderpass*		NewRenderpass(const RenderpassDesc* desc, Result* result) [[gen_rc("")]];
		Texture*		NewTexture(const TextureDesc* desc, StorageMode mode, Result* result);
        Buffer*			NewBuffer(const BufferDesc* desc, StorageMode mode, Result* result);
		RaytracingAS*	NewRaytracingAS();
		Sampler*		NewSampler(const SamplerDesc* desc, Result* result);
        Fence*			NewFence();
        Result			Wait();
    };

    interface Factory {
        Swapchain* NewSwapchain(void* handle, void* reserved);
    };
}