#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include "Metal.hpp"  
#include <iostream>

// from this https://github.com/naleksiev/MTL/blob/master/examples/01_clear.cpp
int main()
{
    const uint32_t width  = 24;
    const uint32_t height = 24;

    auto device = MTL::CreateSystemDefaultDevice();
    assert(device);

    auto *textureDesc = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatRGBA8Unorm, width, height, false);
    textureDesc->setUsage(MTL::TextureUsageRenderTarget);
    auto texture = device->newTexture(textureDesc);
    assert(texture);
    auto commandQueue = device->newCommandQueue();
    assert(commandQueue);
    auto commandBuffer = commandQueue->commandBuffer();
    assert(commandBuffer);
    auto renderPassDesc = MTL::RenderPassDescriptor::alloc()->init();
    auto colorAttachmentDesc = renderPassDesc->colorAttachments()->object(0);

    colorAttachmentDesc->setTexture(texture);
    colorAttachmentDesc->setLoadAction(MTL::LoadActionClear);
    colorAttachmentDesc->setStoreAction(MTL::StoreActionStore);
    colorAttachmentDesc->setClearColor(MTL::ClearColor(1.0, 0.0, 0.0, 0.0));
    renderPassDesc->setRenderTargetArrayLength(1);
    
    auto renderCommandEncoder = commandBuffer->renderCommandEncoder(renderPassDesc);
    assert(renderCommandEncoder);
    renderCommandEncoder->endEncoding();

    auto blitCommandEncoder = commandBuffer->blitCommandEncoder();
    blitCommandEncoder->synchronizeTexture(texture, 0, 0);
    blitCommandEncoder->endEncoding();
    commandBuffer->commit();
    commandBuffer->waitUntilCompleted();
    
    uint32_t data[width * height];
    texture->getBytes(data, width * 4, MTL::Region(0, 0, width, height), 0);

    for (uint32_t i=0; i<width*height; i++)
    {
      std::cout<<(uint32_t)data[i]<<' ';
      assert(data[i] == 0x000000FF);
    }
    std::cout<<'\n';

  device->release();
  textureDesc->release();
  texture->release();
  commandQueue->release();

/*

    

    commandBuffer.Commit();
    commandBuffer.WaitUntilCompleted();

    uint32_t data[width * height];
    texture.GetBytes(data, width * 4, MTL::Region(0, 0, width, height), 0);

    for (uint32_t i=0; i<width*height; i++)
    {
        assert(data[i] == 0x000000FF);
    }*/

    return 0;
}
