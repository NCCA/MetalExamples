# SDLMetal

I'm unsure if this is still the best approach to this.

If using ObjectiveC you can access the metal layer using

```    
const CAMetalLayer *swapchain = (__bridge CAMetalLayer *)SDL_RenderGetMetalLayer(renderer);
```

This does not translate well to the new Metal-cpp API, in particular accessing the surface is not possible as far as I can tell.

```
id<CAMetalDrawable> surface = [swapchain nextDrawable];
```

I have done the following 

1. Grab the layer from the SDLMetal using 

```MTL::Resource *layer= static_cast<MTL::Resource *>( SDL_RenderGetMetalLayer(renderer));```

2. Generate an SDL_Texture the same as the default Metal window using 

```
SDL_Metal_GetDrawableSize(window, &width,&height);
SDL_Texture* sdltexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
```

3. Render using metal to a texture as normal.
4. copy the resultant buffer into the SDL_Texture and blit to screen

```
SDL_LockTexture( sdltexture, NULL, (void**)&pixels, &pitch );
texture->getBytes(pixels, width *4, MTL::Region(0, 0, width, height), 0);
SDL_UnlockTexture( sdltexture );
SDL_RenderCopy(renderer, sdltexture, NULL, NULL);
SDL_RenderPresent(renderer);
```

This works but I am not sure how efficient it is. I guess it is almost the same as using

```
id<CAMetalDrawable> surface = [swapchain nextDrawable];
[buffer presentDrawable:surface];
[buffer commit];
```

I will investigate more when I get a chance.