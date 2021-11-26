#include <SDL.h>
#include <iostream>
#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include "Metal.hpp"
#include <fstream>
#include <string>


// Simple function to load shader from file
// converts to NSString in ASCII encoding
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
  // Basic SDL setup
  SDL_InitSubSystem(SDL_INIT_EVERYTHING);
  // create Window ensure it is a metal one
  SDL_Window *window = SDL_CreateWindow("SDL Metal", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 720, SDL_WINDOW_ALLOW_HIGHDPI  | SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE);
  if(!window)
  {
    std::cout<<"Unable to create Window "<<SDL_GetError()<<'\n';
    exit(EXIT_FAILURE);
  }
  //  -1 to initialize the first drive supporting the requested flags
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,SDL_RENDERER_PRESENTVSYNC);
  if(!renderer)
  {
    std::cout<<"Unable to create Renderer "<<SDL_GetError()<<'\n';
    exit(EXIT_FAILURE);
  }

  // now we need to get the Render Layer we need to cast the void * from SDL to an MTL::Resource
  // from this we can get the actual device we are using.
  MTL::Resource *layer= static_cast<MTL::Resource *>( SDL_RenderGetMetalLayer(renderer));
  auto device = layer->device();
  // get window size to generate our textures
  int width,height;
  SDL_Metal_GetDrawableSize(window, &width,&height);
  // Build Metal texture (this is where we are going to render to with metal)
  auto *textureDesc = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatRGBA8Unorm, width, height, false);
  textureDesc->setUsage(MTL::TextureUsageRenderTarget);
  auto texture = device->newTexture(textureDesc);
  // Generate SDL texture, this will copy the metal render texture then blit to the renderere
  // Must be the same size and format, at presetn whilst saying RGBA everything seems to be in BGRA!
  SDL_Texture* sdltexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);

  // Load in the shaders we need to set some default options and error handlers.
  // note these must be released when finished with as they are owned by us as calling alloc
  auto *shader=loadShader("shader.metal");
  auto *compileOptions = MTL::CompileOptions::alloc()->init();

  auto *errorDictionary = NS::Dictionary::dictionary();
  auto *errorMessages=NS::Error::alloc()->init(NS::CocoaErrorDomain,99,errorDictionary);
  auto library = device->newLibrary(shader,compileOptions,&errorMessages); 
  // get the shader functions from the compiled shaders 
  auto vertFunc = library->newFunction(NS::String::string("vertFunc",NS::ASCIIStringEncoding));
  auto fragFunc = library->newFunction(NS::String::string("fragFunc",NS::ASCIIStringEncoding));
  std::cout<<"Error Description "<<errorMessages->localizedDescription()<<'\n';
  std::cout<<"localizedRecoverySuggestion "<<errorMessages->localizedRecoverySuggestion()<<'\n';
  std::cout<<"localizedFailureReason "<<errorMessages->localizedFailureReason()<<'\n';
  // Now build a render pipline
  auto *renderPipelineDesc = MTL::RenderPipelineDescriptor::alloc()->init();
  renderPipelineDesc->setVertexFunction(vertFunc);
  renderPipelineDesc->setFragmentFunction(fragFunc);
  renderPipelineDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
  auto * renderPipelineState = device->newRenderPipelineState(renderPipelineDesc,&errorMessages);

  std::cout<<"Error Description "<<errorMessages->localizedDescription()<<'\n';
  std::cout<<"localizedRecoverySuggestion "<<errorMessages->localizedRecoverySuggestion()<<'\n';
  std::cout<<"localizedFailureReason "<<errorMessages->localizedFailureReason()<<'\n';
  // Vertex data, [Vertex x,y,z,w] [Colour B,G,R,A]
  const float vertexData[] =
  {
      0.0f,  1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0,
    -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
      1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
  };
  // create a buffer for the vertex data
  auto *vertexBuffer = device->newBuffer(vertexData, sizeof(vertexData), MTL::CPUCacheModeDefaultCache);
  // create a new command queue to register our commands
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
      } // event
    } // end poll
    // generate a command buffer
    auto *commandBuffer = commandQueue->commandBuffer();
    // get the render pass info an set the details
    auto renderPassDesc = MTL::RenderPassDescriptor::alloc()->init();
    auto colorAttachmentDesc = renderPassDesc->colorAttachments()->object(0);
    colorAttachmentDesc->setTexture(texture);
    colorAttachmentDesc->setLoadAction(MTL::LoadActionClear);
    colorAttachmentDesc->setStoreAction(MTL::StoreActionStore);
    colorAttachmentDesc->setClearColor(MTL::ClearColor(0.0f, 0.8f, 0.8f, 0.8f)); // BGRA?
    renderPassDesc->setRenderTargetArrayLength(1);
    // encode our render command and draw 
    auto renderCommandEncoder = commandBuffer->renderCommandEncoder(renderPassDesc);
    renderCommandEncoder->setRenderPipelineState(renderPipelineState);
    renderCommandEncoder->setVertexBuffer(vertexBuffer, 0, 0);
    renderCommandEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle,  NS::UInteger(0),  NS::UInteger(3));
    renderCommandEncoder->endEncoding();
    // finally blit the result to our texture
    auto blitCommandEncoder = commandBuffer->blitCommandEncoder();
    blitCommandEncoder->synchronizeTexture(texture, 0, 0);
    blitCommandEncoder->endEncoding();
    commandBuffer->commit();
    commandBuffer->waitUntilCompleted();
    // now copy to the SDL texture and present to the renderer
    uint32_t* pixels;
    int pitch;

    SDL_LockTexture( sdltexture, NULL, (void**)&pixels, &pitch );
    texture->getBytes(pixels, width *4, MTL::Region(0, 0, width, height), 0);
    SDL_UnlockTexture( sdltexture );
    SDL_RenderCopy(renderer, sdltexture, NULL, NULL);
    SDL_RenderPresent(renderer);
    // this is the only per frame allocation so release.
    renderPassDesc->release();
  }// end loop

  renderPipelineDesc->release();
  errorMessages->release();
  compileOptions->release();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

return EXIT_SUCCESS;
}