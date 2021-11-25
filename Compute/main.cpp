#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include "Metal.hpp"  
#include <iostream>
#include <cstdlib>
// based on https://github.com/naleksiev/mtlpp/blob/master/examples/03_compute.cpp

int main()
{
  auto device = MTL::CreateSystemDefaultDevice();


    auto *shaderSrc=NS::String::string(
      R"""(
        #include <metal_stdlib>
        using namespace metal;

        kernel void sqr(
            const device float *vIn [[ buffer(0) ]],
            device float *vOut [[ buffer(1) ]],
            uint id[[ thread_position_in_grid ]])
        {
            vOut[id] = vIn[id] * vIn[id];
        }
    )""",NS::ASCIIStringEncoding);

    auto *compileOptions = MTL::CompileOptions::alloc()->init();
    auto *errorDictionary = NS::Dictionary::dictionary();
    auto *errorMessages=NS::Error::alloc()->init(NS::CocoaErrorDomain,99,errorDictionary);
    auto library = device->newLibrary(shaderSrc,compileOptions,&errorMessages); 
    assert(library);
    auto sqrFunc = library->newFunction(NS::String::string("sqr",NS::ASCIIStringEncoding));
    assert(sqrFunc);
    auto *computePipelineState = device->newComputePipelineState(sqrFunc,&errorMessages);
    assert(computePipelineState);
    auto *commandQueue = device->newCommandQueue();
    assert(commandQueue);

    const uint32_t dataCount = 6;

    auto *inBuffer = device->newBuffer(sizeof(float) * dataCount, MTL::StorageModeManaged);
    assert(inBuffer);

    auto *outBuffer = device->newBuffer(sizeof(float) * dataCount, MTL::StorageModeManaged);
    assert(outBuffer);

    for (uint32_t i=0; i<4; i++)
    {
        // update input data
        {
            float* inData = static_cast<float*>(inBuffer->contents());
            for (uint32_t j=0; j<dataCount; j++)
                inData[j] = 10 * i + j;
            inBuffer->didModifyRange(NS::Range(0, sizeof(float) * dataCount));
        }

        auto commandBuffer = commandQueue->commandBuffer();
        assert(commandBuffer);

        auto *commandEncoder = commandBuffer->computeCommandEncoder();
        commandEncoder->setBuffer(inBuffer, 0, 0);
        commandEncoder->setBuffer(outBuffer, 0, 1);
        commandEncoder->setComputePipelineState(computePipelineState);
        commandEncoder->dispatchThreadgroups(
            MTL::Size(1, 1, 1),
            MTL::Size(dataCount, 1, 1));
        commandEncoder->endEncoding();

        auto blitCommandEncoder = commandBuffer->blitCommandEncoder();
        blitCommandEncoder->synchronizeResource(outBuffer);
        blitCommandEncoder->endEncoding();

        commandBuffer->commit();
        commandBuffer->waitUntilCompleted();

        // read the data
        {
            float* inData = static_cast<float*>(inBuffer->contents());
            float* outData = static_cast<float*>(outBuffer->contents());
            for (uint32_t j=0; j<dataCount; j++)
                printf("sqr(%g) = %g\n", inData[j], outData[j]);
        }
    }


    return EXIT_SUCCESS;
}
