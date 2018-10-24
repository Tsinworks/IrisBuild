namespace ngfx {
    enum Result {
        Ok,
        Failed,
        ParamError,
        DeviceNotFound
    };

	enum HardwareTier {
		Software,
		Graphics,
		Compute,
		Raytracing
	};

	enum PipelineType {
		Graphics,
		Compute,
		RayTracing
	};

    [[vulkan("VkFormat"), metal("MTLPixelFormat")]]
    enum PixelFormat {
        Invalid,
        
        A8Unorm                     [[metal("MTLPixelFormatA8Unorm")]],
        R8Unorm                     [[metal("MTLPixelFormatR8Unorm")]],
        R8Unorm_sRGB                [[metal("MTLPixelFormatR8Unorm_sRGB")]],
        R8Snorm                     [[metal("MTLPixelFormatR8Snorm")]],
        R8Uint                      [[metal("MTLPixelFormatR8Uint")]],
        R8Sint                      [[metal("MTLPixelFormatR8Sint")]],

        R16Unorm                    [[metal("MTLPixelFormatR16Unorm")]],
        R16Snorm                    [[metal("MTLPixelFormatR16Snorm")]],
        R16Uint                     [[metal("MTLPixelFormatR16Uint")]],
        R16Sint                     [[metal("MTLPixelFormatR16Sint")]],
        R16Float                    [[metal("MTLPixelFormatR16Float")]],
        RG8Unorm,
        RG8Unorm_sRGB,
        RG8Snorm,
        RG8Uint,
        RG8Sint,

        R32Uint,
        R32Sint,
        R32Float,
        RG16Unorm,
        RG16Snorm,
        RG16Uint,
        RG16Sint,
        RG16Float,
        RGBA8Unorm,
        RGBA8Unorm_sRGB,
        RGBA8Snorm,
        RGBA8Uint,
        RGBA8Sint,

        BGRA8Unorm,
        BGRA8Unorm_sRGB,

        RG11B10Float                [[vulkan("VK_FORMAT_B10G11R11_UFLOAT_PACK32")]],
        RGB9E5Float,
        
        RG32Uint,
        RG32Sint,
        RG32Float,
        RGBA16Unorm,
        RGBA16Snorm,
        RGBA16Uint                  [[vulkan("VK_FORMAT_R16G16B16A16_UINT")]],
        RGBA16Sint,
        RGBA16Float,

        RGBA32Uint,
        RGBA32Sint,
        RGBA32Float                 [[vulkan("VK_FORMAT_R32G32B32A32_SFLOAT")]],

        Depth16Unorm                [[metal("MTLPixelFormatDepth16Unorm")]],
        Depth32Float                [[metal("MTLPixelFormatDepth32Float")]],
        Stencil8                    [[metal("MTLPixelFormatStencil8")]],
        Depth24UnormStencil8        [[metal("MTLPixelFormatDepth24Unorm_Stencil8")]],
        Depth32FloatStencil8        [[metal("MTLPixelFormatDepth32Float_Stencil8")]]
    };
    [[vulkan("VkSampleCountFlagBits")]]
    enum MultisampleFlags {
        None,
        MS1X [[vulkan("VK_SAMPLE_COUNT_1_BIT")]],
        MS2X [[vulkan("VK_SAMPLE_COUNT_2_BIT")]],
        MS4X [[vulkan("VK_SAMPLE_COUNT_4_BIT")]],
        MS8X [[vulkan("VK_SAMPLE_COUNT_8_BIT")]],
        MS16X [[vulkan("VK_SAMPLE_COUNT_16_BIT")]]
    };
    [[vulkan("VkAttachmentLoadOp"),metal("MTLLoadAction")]]
    enum LoadAction {
        Load        [[vulkan("VK_ATTACHMENT_LOAD_OP_LOAD"),         metal("MTLLoadActionLoad")]],
        Clear       [[vulkan("VK_ATTACHMENT_LOAD_OP_CLEAR"),        metal("MTLLoadActionClear")]],
        DontCare    [[vulkan("VK_ATTACHMENT_LOAD_OP_DONT_CARE"),    metal("MTLLoadActionDontCare")]]
    };
    [[vulkan("VkAttachmentStoreOp"),metal("MTLStoreAction")]]
    enum StoreAction {
        Store       [[vulkan("VK_ATTACHMENT_STORE_OP_STORE"),       metal("MTLStoreActionStore")]],
        DontCare    [[vulkan("VK_ATTACHMENT_STORE_OP_DONT_CARE"),   metal("MTLStoreActionDontCare")]]
    };
    [[vulkan("VkCompareOp"), metal("MTLCompareFunction")]]
    enum ComparisonFunction {
        Never       [[vulkan("VK_COMPARE_OP_NEVER"),            metal("MTLCompareFunctionNever")]],
        Less        [[vulkan("VK_COMPARE_OP_LESS"),             metal("MTLCompareFunctionLess")]],
        Equal       [[vulkan("VK_COMPARE_OP_EQUAL"),            metal("MTLCompareFunctionNever")]],
        LessEqual   [[vulkan("VK_COMPARE_OP_LESS_OR_EQUAL"),    metal("MTLCompareFunctionLessEqual")]],
        Greater     [[vulkan("VK_COMPARE_OP_GREATER"),          metal("MTLCompareFunctionGreater")]],
        NotEqual    [[vulkan("VK_COMPARE_OP_NOT_EQUAL"),        metal("MTLCompareFunctionNotEqual")]],
        GreaterEqual[[vulkan("VK_COMPARE_OP_GREATER_OR_EQUAL"), metal("MTLCompareFunctionGreaterEqual")]],
        Always      [[vulkan("VK_COMPARE_OP_ALWAYS"),           metal("MTLCompareFunctionAlways")]]
    };
    [[vulkan("VkBlendOp"), metal("MTLBlendOperation")]]
    enum BlendOperation {
        Add     [[vulkan("VK_BLEND_OP_ADD"),                metal("MTLBlendOperationAdd")]],
        Sub     [[vulkan("VK_BLEND_OP_SUBTRACT"),           metal("MTLBlendOperationSubtract")]],
        RevSub  [[vulkan("VK_BLEND_OP_REVERSE_SUBTRACT"),   metal("MTLBlendOperationReverseSubtract")]],
        Min     [[vulkan("VK_BLEND_OP_MIN"),                metal("MTLBlendOperationMin")]],
        Max     [[vulkan("VK_BLEND_OP_MAX"),                metal("MTLBlendOperationMax")]]
    };
    [[vulkan("VkBlendFactor"), metal("MTLBlendFactor")]]
    enum BlendFactor {
        Zero                [[vulkan("VK_BLEND_FACTOR_ZERO"),                   metal("MTLBlendFactorZero")]],
        One                 [[vulkan("VK_BLEND_FACTOR_ONE"),                    metal("MTLBlendFactorOne")]],
        SrcColor            [[vulkan("VK_BLEND_FACTOR_SRC_COLOR"),              metal("MTLBlendFactorSourceColor")]],
        OneMinusSrcColor    [[vulkan("VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR"),    metal("MTLBlendFactorOneMinusSourceColor")]],
        SrcAlpha            [[vulkan("VK_BLEND_FACTOR_SRC_ALPHA"),              metal("MTLBlendFactorSourceAlpha")]],
        OneMinusSrcAlpha    [[vulkan("VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA"),    metal("MTLBlendFactorOneMinusSourceAlpha")]],
        DestColor           [[vulkan("VK_BLEND_FACTOR_DST_COLOR"),              metal("MTLBlendFactorDestinationColor")]],
        OneMinusDestColor   [[vulkan("VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR"),    metal("MTLBlendFactorOneMinusDestinationColor")]],
        DestAlpha           [[vulkan("VK_BLEND_FACTOR_DST_ALPHA"),              metal("MTLBlendFactorDestinationAlpha")]],
        OneMinusDestAlpha   [[vulkan("VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA"),    metal("MTLBlendFactorOneMinusDestinationAlpha")]]
    };
    [[vulkan("VkStencilOp"),metal("MTLStencilOperation")]]
    enum StencilOperation {
        Keep                [[vulkan("VK_STENCIL_OP_KEEP"),                 metal("MTLStencilOperationKeep")]],
        Zero                [[vulkan("VK_STENCIL_OP_ZERO"),                 metal("MTLStencilOperationZero")]],
        Replace             [[vulkan("VK_STENCIL_OP_REPLACE"),              metal("MTLStencilOperationReplace")]],
        IncrementAndClamp   [[vulkan("VK_STENCIL_OP_INCREMENT_AND_CLAMP"),  metal("MTLStencilOperationIncrementClamp")]],
        DecrementAndClamp   [[vulkan("VK_STENCIL_OP_DECREMENT_AND_CLAMP"),  metal("MTLStencilOperationDecrementClamp")]],
        Invert              [[vulkan("VK_STENCIL_OP_INVERT"),               metal("MTLStencilOperationInvert")]],
        IncrementWrap       [[vulkan("VK_STENCIL_OP_INCREMENT_AND_WRAP"),   metal("MTLStencilOperationIncrementWrap")]],
        DecrementWrap       [[vulkan("VK_STENCIL_OP_DECREMENT_AND_WRAP"),   metal("MTLStencilOperationDecrementWrap")]]
    };

    enum DepthWriteMask {
        Zero,
        All
    };

    enum ResourceState {
        VertexBuffer,
        UniformBuffer,
        UnorderedAccess,
		RaytraceAccelerationStructure
    };

	enum StorageMode {
		Auto,
		OnGpu,
		OnCpu,
		Shared
	};

    enum TextureDim {
        Tex1D,
        Tex2D,
        Tex3D
    };

    [[vulkan("VkPolygonMode"), metal("MTLTriangleFillMode")]]
    enum FillMode {
        Line [[vulkan("VK_POLYGON_MODE_LINE"), metal("MTLTriangleFillModeLines")]],
        Fill [[vulkan("VK_POLYGON_MODE_FILL"), metal("MTLTriangleFillModeFill")]]
    };

    [[vulkan("VkCullModeFlagBits"), metal("MTLCullMode")]]
    enum CullMode {
        None        [[vulkan("VK_CULL_MODE_NONE"), metal("MTLCullModeNone")]],
        Front       [[vulkan("VK_CULL_MODE_FRONT_BIT"), metal("MTLCullModeFront")]],
        Back        [[vulkan("VK_CULL_MODE_BACK_BIT"), metal("MTLCullModeBack")]]
    };
    [[metal("MTLSamplerMinMagFilter")]]
    enum FilterMode {
        Point       [[metal("MTLSamplerMinMagFilterNearest")]],
        Linear      [[metal("MTLSamplerMinMagFilterLinear")]]
    };

    [[vulkan("VkSamplerAddressMode"), metal("MTLSamplerAddressMode")]]
    enum SamplerAddressMode {
        Wrap        [[vulkan("VK_SAMPLER_ADDRESS_MODE_REPEAT"), metal("MTLSamplerAddressModeRepeat")]],
        Mirror      [[vulkan("VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT"), metal("MTLSamplerAddressModeMirrorRepeat")]],
        Clamp       [[vulkan("VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE"), metal("MTLSamplerAddressModeClampToEdge")]],
        Border      [[vulkan("VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER"), metal("MTLSamplerAddressModeClampToBorderColor")]],
        MirrorOnce  [[vulkan("VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE"), metal("MTLSamplerAddressModeMirrorClampToEdge")]]
    };
    [[metal("MTLPrimitiveType")]]
    enum PrimitiveType {
        Points          [[metal("MTLPrimitiveTypePoint")]],
        Lines           [[metal("MTLPrimitiveTypeLine")]],
        LineStrips      [[metal("MTLPrimitiveTypeLineStrip")]],
        Triangles       [[metal("MTLPrimitiveTypeTriangle")]],
        TriangleStrips  [[metal("MTLPrimitiveTypeTriangleStrip")]]
    };
	[[vulkan("VkVertexInputRate")]]
	enum VertexInputRate {
		PerVertex		[[vulkan("VK_VERTEX_INPUT_RATE_VERTEX")]],
		PerInstance		[[vulkan("VK_VERTEX_INPUT_RATE_INSTANCE")]]
	};

	enum ShaderStage {
		Vertex,
		Pixel,
		Geometry,
		Compute,
		Domain,
		Hull,
		RayGenerate,
		AnyHit,
		ClosetHit,
		MissHit,
		Intersect
	};
}