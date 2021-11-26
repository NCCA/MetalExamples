#include <SDL.h>
#include <iostream>
#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include "Metal.hpp"
#include <fstream>
#include <string>

NS::String *loadShader(const std::string &_fname)
{
  std::ifstream shaderSource(_fname.data());
  if (!shaderSource.is_open())
  {
   std::cerr<<"Unable to open shader file "<<_fname<<'\n';
   exit(EXIT_FAILURE);
  }
  // now read in the data
  auto source =  std::string((std::istreambuf_iterator<char>(shaderSource)), std::istreambuf_iterator<char>());
  shaderSource.close();
  source+='\0';

  return NS::String::string(source.data(),NS::ASCIIStringEncoding);
}

int main (int argc, char *args[])
{
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    SDL_setenv("METAL_DEVICE_WRAPPER_TYPE", "1", 0);

    SDL_Window *window = SDL_CreateWindow("SDL Metal", -1, -1, 1024, 720, SDL_WINDOW_ALLOW_HIGHDPI  | SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC );
    int width,height;

    MTL::Resource *layer= static_cast<MTL::Resource *>( SDL_RenderGetMetalLayer(renderer));
    auto device = layer->device();
     SDL_Metal_GetDrawableSize(window, &width,&height);

    //auto device = MTL::CreateSystemDefaultDevice();

    auto *textureDesc = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatRGBA8Unorm, width, height, false);
    textureDesc->setUsage(MTL::TextureUsageRenderTarget);
    auto texture = device->newTexture(textureDesc);
    SDL_Texture* sdltexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);

    auto *shader=loadShader("shader.metal");
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
    renderPipelineDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
    auto * renderPipelineState = device->newRenderPipelineState(renderPipelineDesc,&errorMessages);

    std::cout<<"Error Description "<<errorMessages->localizedDescription()<<'\n';
    std::cout<<"localizedRecoverySuggestion "<<errorMessages->localizedRecoverySuggestion()<<'\n';
    std::cout<<"localizedFailureReason "<<errorMessages->localizedFailureReason()<<'\n';

    const float vertexData[] =
    {
         0.0f,  1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0,
        -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
    };
    auto *vertexBuffer = device->newBuffer(vertexData, sizeof(vertexData), MTL::CPUCacheModeDefaultCache);
    auto *commandQueue = device->newCommandQueue();


    bool quit = false;
    SDL_Event e;

    while (!quit) 
    {
        while (SDL_PollEvent(&e) != 0) 
        {
            switch (e.type) 
            {
                case SDL_QUIT: quit = true; break;
                case SDL_KEYDOWN :
                  switch (e.key.keysym.sym )
                  {
                    case SDLK_ESCAPE : quit=true; break;
                  }
                break;
            }
        } // end poll
      auto *commandBuffer = commandQueue->commandBuffer();

      auto renderPassDesc = MTL::RenderPassDescriptor::alloc()->init();
      auto colorAttachmentDesc = renderPassDesc->colorAttachments()->object(0);

      colorAttachmentDesc->setTexture(texture);
      colorAttachmentDesc->setLoadAction(MTL::LoadActionClear);
      colorAttachmentDesc->setStoreAction(MTL::StoreActionStore);
      colorAttachmentDesc->setClearColor(MTL::ClearColor(0.0f, 0.8f, 0.8f, 0.8f)); // BGRA?
      renderPassDesc->setRenderTargetArrayLength(1);

      auto renderCommandEncoder = commandBuffer->renderCommandEncoder(renderPassDesc);
      renderCommandEncoder->setRenderPipelineState(renderPipelineState);
      renderCommandEncoder->setVertexBuffer(vertexBuffer, 0, 0);
      renderCommandEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle,  NS::UInteger(0),  NS::UInteger(3));
      renderCommandEncoder->endEncoding();

      auto blitCommandEncoder = commandBuffer->blitCommandEncoder();
      blitCommandEncoder->synchronizeTexture(texture, 0, 0);
      blitCommandEncoder->endEncoding();
      commandBuffer->commit();
      commandBuffer->waitUntilCompleted();
      uint32_t* pixels;
      int pitch;

      SDL_LockTexture( sdltexture, NULL, (void**)&pixels, &pitch );
      texture->getBytes(pixels, width *4, MTL::Region(0, 0, width, height), 0);
      SDL_UnlockTexture( sdltexture );
      SDL_RenderCopy(renderer, sdltexture, NULL, NULL);
      SDL_RenderPresent(renderer);

    }// end loop




    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}