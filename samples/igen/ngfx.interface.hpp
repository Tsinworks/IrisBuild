#include "ngfx.structs.hpp"

namespace ngfx {
	interface Device;

    interface Blob {
        const void*			Data() const;
        uint64				Length() const;
    };
    interface Function {
        const char*			EntryPoint();
        const Blob*			Bundle();
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
		Function*			function;
		uint32				deviceMask;
    };
    struct RaytracePipelineDesc {
		uint32				maxTraceRecurseDepth;
		array<Function*>	functions;
    };
	struct RenderpassDesc {

	};
    interface Resource {
		void*				Map(uint64 offset, uint64 size);
		void				Unmap(void* addr);
		void				SetName(string name);
    };
	interface TextureView;
	[[vulkan("VkImage"), metal("id<MTLTexture>")]]
    interface Texture : Resource {
		PixelFormat			Format() const;
		TextureView*		NewView(Result* result);
    };
	interface TextureView {
		const Texture*		GetTexture() const;
		TextureUsage		GetUsage() const;
	};
	interface BufferView;
	[[vulkan("VkBuffer"), metal("id<MTLBuffer>")]]
    interface Buffer : Resource {
		BufferView*			NewView(Result* result);
    };
	interface BufferView {
		const Buffer*		GetBuffer() const;
		BufferUsage			GetUsage() const;
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
	[[vulkan("VkSwapchain")]]
    interface Swapchain {
		Texture*			CurrentTexture();
    };
	[[transient("frame")]]
	interface BindGroup {
		void				SetSampler(uint32 id, ShaderStage stage, const Sampler* texture);
		void				SetTexture(uint32 id, ShaderStage stage, const TextureView* texture);
		void				SetBuffer(uint32 id, ShaderStage stage, const BufferView* texture);
	};
    interface Pipeline {
		BindGroup*			NewBindGroup(Result* result);
		Device*				GetDevice();
    };
    interface RenderPipeline : Pipeline {

    };
	[[vulkan("VkRenderpass")]]
	interface Renderpass {
		RenderPipeline*		NewRenderPipeline(const RenderPipelineDesc* desc, Result* result);
	};
    interface ComputePipeline : Pipeline {

    };
    interface RaytracePipeline : Pipeline {

    };
	[[vulkan("VkCommandBuffer"), metal("id<MTLCommandBuffer>")]]
    interface CommandBuffer {
        RenderEncoder*		NewRenderEncoder(Result* result);
        ComputeEncoder*		NewComputeEncoder(Result* result);
		Result				NewBlitEncoder();
        Result				NewParallelRenderEncoder();
        RaytraceEncoder*	NewRaytraceEncoder(Result* result);
        Result				Commit();
    };
	[[vulkan("VkQueue"), metal("id<MTLQueue>")]]
    interface CommandQueue {
		CommandBuffer*		NewCommandBuffer() [[transient("true")]];
    };
    interface CommandEncoder {
		void				SetPipeline(Pipeline* pipeline);
		void				SetBindGroup(const BindGroup* bindGroup);
        void				EndEncode();
    };
    interface RenderEncoder : CommandEncoder {
        void				Draw();
        void				Present(Swapchain* swapchain);
    };
    interface ComputeEncoder : CommandEncoder {
        void				Dispatch(int x, int y, int z);
    };
    interface RaytraceEncoder : CommandEncoder {
		void				BuildAS();
        void				TraceRay(int width, int height);
    };
	[[vulkan("VkFence"), metal("id<MTLFence>")]]
    interface Fence {
        void				Signal();
    };
	[[vulkan("VkDevice"), metal("id<MTLDevice>")]]
    interface Device {
		CommandQueue*		NewQueue();
        Shader*				NewShader();
        Renderpass*			NewRenderpass(const RenderpassDesc* desc, Result* result) [[gen_rc("")]];
		ComputePipeline*	NewComputePipeline(const ComputePipelineDesc* desc, Result* result);
		RaytracePipeline*	NewRaytracePipeline(const RaytracePipelineDesc* desc, Result* result);
		Texture*			NewTexture(const TextureDesc* desc, StorageMode mode, Result* result);
        Buffer*				NewBuffer(const BufferDesc* desc, StorageMode mode, Result* result);
		RaytracingAS*		NewRaytracingAS(const RaytracingASDesc* rtDesc, Result* result);
		Sampler*			NewSampler(const SamplerDesc* desc, Result* result);
        Fence*				NewFence();
        Result				Wait();
    };
    interface Factory {
        Swapchain*			NewSwapchain(void* handle, void* reserved);
    };
}