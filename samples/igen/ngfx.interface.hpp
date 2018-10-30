#include "ngfx.structs.hpp"

namespace ngfx {
	interface Device;
    [[refcount("true")]]
    interface LabeledObject
    {
        void                SetLabel(const char* label);
        const char*         Label() const;
    };
    [[refcount("true")]]
    interface Blob {
        const void*			Data() const;
        uint64				Length() const;
    };
    [[refcount("true")]]
    interface Function {
        const char*			EntryPoint() const;
        const Blob*			Bundle() const;
    };
    struct RenderPipelineDesc {
        RasterizerState     rasterizer;
        BlendState          blend;
        DepthStencilState   depthStencil;
        VertexInputState    input;

        PixelFormat         depthStencilFormat;

		Function*			vertex;
		Function*			pixel;
		Function*			geometry;
		Function*			domain;
		Function*			hull;
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
    interface Resource : LabeledObject {
		void*				Map(uint64 offset, uint64 size);
		void				Unmap(void* addr);
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
    [[refcount("true")]]
	interface RaytracingAS : LabeledObject {

	};
	[[vulkan("VkSampler"), metal("id<MTLSampler>")]]
    interface Sampler : LabeledObject {

    };
    interface Shader : LabeledObject {

    };
	[[vulkan("VkFramebuffer"),refcount("true")]]
    interface Framebuffer : LabeledObject {

    };
	[[vulkan("VkSwapchain"),refcount("true")]]
    interface Swapchain {
		Texture*			CurrentTexture();
    };
	[[transient("frame"), refcount("true")]]
	interface BindGroup {
		void				SetSampler(uint32 id, ShaderStage stage, const Sampler* texture);
		void				SetTexture(uint32 id, ShaderStage stage, const TextureView* texture);
		void				SetBuffer(uint32 id, ShaderStage stage, const BufferView* texture);
	};
    interface Pipeline : LabeledObject {
		BindGroup*			NewBindGroup(Result* result);
		Device*				GetDevice();
    };
    interface RenderPipeline : Pipeline {

    };
	[[vulkan("VkRenderpass"), refcount("true")]]
	interface Renderpass {
		RenderPipeline*		NewRenderPipeline(const RenderPipelineDesc* desc, Result* result);
	};
    interface ComputePipeline : Pipeline {

    };
    interface RaytracePipeline : Pipeline {
        
    };
	interface RenderEncoder;
	interface ComputeEncoder;
	interface RaytraceEncoder;
	[[vulkan("VkCommandBuffer"), metal("id<MTLCommandBuffer>")]]
    interface CommandBuffer : LabeledObject {
        RenderEncoder *		NewRenderEncoder(Result* result);
        ComputeEncoder*		NewComputeEncoder(Result* result);
		Result				NewBlitEncoder();
        Result				NewParallelRenderEncoder();
        RaytraceEncoder*	NewRaytraceEncoder(Result* result);
        Result				Commit();
    };
	[[refcount("true"), vulkan("VkQueue"), metal("id<MTLQueue>")]]
    interface CommandQueue {
		CommandBuffer*		NewCommandBuffer() [[transient("true")]];
    };
    interface CommandEncoder : LabeledObject{
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
    interface Fence : LabeledObject {
        void				Signal();
    };
	[[vulkan("VkDevice"), metal("id<MTLDevice>")]]
    interface Device : LabeledObject {
		CommandQueue*		NewQueue();
        Shader*				NewShader();
        Renderpass*			NewRenderpass(const RenderpassDesc* desc, Result* result) [[gen_rc("true")]];
		ComputePipeline*	NewComputePipeline(const ComputePipelineDesc* desc, Result* result)[[gen_rc("true")]];
		RaytracePipeline*	NewRaytracePipeline(const RaytracePipelineDesc* desc, Result* result)[[gen_rc("true")]];
		Texture*			NewTexture(const TextureDesc* desc, StorageMode mode, Result* result)[[gen_rc("true")]];
        Buffer*				NewBuffer(const BufferDesc* desc, StorageMode mode, Result* result)[[gen_rc("true")]];
		RaytracingAS*		NewRaytracingAS(const RaytracingASDesc* rtDesc, Result* result)[[gen_rc("true")]];
		Sampler*			NewSampler(const SamplerDesc* desc, Result* result)[[gen_rc("true")]];
        Fence*				NewFence();
        Result				Wait();
    };
    [[refcount("true")]]
    interface Factory {
        Swapchain*			NewSwapchain(void* handle, void* reserved);
    };
}