#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include "Metal.hpp"  
#include <iostream>
#include <cstdlib>
// based on https://github.com/naleksiev/mtlpp/blob/master/examples/02_triangle.cpp
int main()
{
    const uint32_t width  = 16;
    const uint32_t height = 16;
    auto device = MTL::CreateSystemDefaultDevice();

    auto *textureDesc = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatR8Unorm, width, height, false);
    textureDesc->setUsage(MTL::TextureUsageRenderTarget);
    auto texture = device->newTexture(textureDesc);

    auto *shader=NS::String::string(
R"""(
#include <metal_stdlib>
using namespace metal;

vertex float4 vertFunc(const device packed_float3* vertexArray [[buffer(0)]],unsigned int vID[[vertex_id]])
{
    return float4(vertexArray[vID], 1.0);
}

fragment half4 fragFunc ()
{
    return half4(1.0);
}
)""",NS::ASCIIStringEncoding);
    auto *compileOptions = MTL::CompileOptions::alloc()->init();

    auto *errorDictionary = NS::Dictionary::dictionary();
    auto *errorMessages=NS::Error::alloc()->init(NS::CocoaErrorDomain,99,errorDictionary);
    auto library = device->newLibrary(shader,compileOptions,&errorMessages); 
    auto vertFunc = library->newFunction(NS::String::string("vertFunc",NS::ASCIIStringEncoding));
    auto fragFunc = library->newFunction(NS::String::string("fragFunc",NS::ASCIIStringEncoding));
    std::cout<<"Error Description "<<errorMessages->localizedDescription()<<'\n';
    std::cout<<"localizedRecoverySuggestion "<<errorMessages->localizedRecoverySuggestion()<<'\n';
    std::cout<<"localizedFailureReason "<<errorMessages->localizedFailureReason()<<'\n';

    auto *renderPipelineDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    renderPipelineDesc->setVertexFunction(vertFunc);
    renderPipelineDesc->setFragmentFunction(fragFunc);
    renderPipelineDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatR8Unorm);
    auto * renderPipelineState = device->newRenderPipelineState(renderPipelineDesc,&errorMessages);

    std::cout<<"Error Description "<<errorMessages->localizedDescription()<<'\n';
    std::cout<<"localizedRecoverySuggestion "<<errorMessages->localizedRecoverySuggestion()<<'\n';
    std::cout<<"localizedFailureReason "<<errorMessages->localizedFailureReason()<<'\n';
    assert(renderPipelineState);

    const float vertexData[] =
    {
         0.0f,  1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
    };
    auto *vertexBuffer = device->newBuffer(vertexData, sizeof(vertexData), MTL::CPUCacheModeDefaultCache);
    assert(vertexBuffer);
    auto *commandQueue = device->newCommandQueue();
    assert(commandQueue);
    auto *commandBuffer = commandQueue->commandBuffer();
    assert(commandBuffer);

    auto renderPassDesc = MTL::RenderPassDescriptor::alloc()->init();
    auto colorAttachmentDesc = renderPassDesc->colorAttachments()->object(0);

    colorAttachmentDesc->setTexture(texture);
    colorAttachmentDesc->setLoadAction(MTL::LoadActionClear);
    colorAttachmentDesc->setStoreAction(MTL::StoreActionStore);
    colorAttachmentDesc->setClearColor(MTL::ClearColor(0.0, 0.0, 0.0, 0.0));
    renderPassDesc->setRenderTargetArrayLength(1);

    auto renderCommandEncoder = commandBuffer->renderCommandEncoder(renderPassDesc);
    assert(renderCommandEncoder);
    renderCommandEncoder->setRenderPipelineState(renderPipelineState);
    renderCommandEncoder->setVertexBuffer(vertexBuffer, 0, 0);
    renderCommandEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle,  NS::UInteger(0),  NS::UInteger(3));
    renderCommandEncoder->endEncoding();

    auto blitCommandEncoder = commandBuffer->blitCommandEncoder();
    blitCommandEncoder->synchronizeTexture(texture, 0, 0);
    blitCommandEncoder->endEncoding();
    commandBuffer->commit();
    commandBuffer->waitUntilCompleted();


    uint8_t data[width * height];
    texture->getBytes(data, width , MTL::Region(0, 0, width, height), 0);

    for (uint32_t y=0; y<height; y++)
    {
        for (uint32_t x=0; x<width; x++)
        {
            printf("%02X ", data[x + y * width]);
        }
        printf("\n");
    }

    return EXIT_SUCCESS;
}
