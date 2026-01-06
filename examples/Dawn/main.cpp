// Copyright 2019 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This file is derived from https://github.com/kainino0x/webgpu-cross-platform-demo/blob/main/main.cpp
// and heavily modified for this example

#include <webgpu/webgpu_cpp.h>
#include <emscripten/html5.h>
#include <functional>

const uint32_t kWidth = 300;
const uint32_t kHeight = 150;

void terminate(std::string_view iMessage)
{
  printf("%s\n; Exiting cleanly", iMessage.data());
  exit(0);
}

class GPU
{
public:
  explicit GPU(wgpu::Instance iInstance) : fInstance{std::move(iInstance)} {}

  static void asyncCreate(std::function<void(std::shared_ptr<GPU> iGPU)> onCreated,
                          std::function<void(wgpu::StringView)> const &onError);

  wgpu::Instance &instance() { return fInstance; }
  wgpu::Device &device() { return fDevice; }
  wgpu::Queue &queue() { return fQueue; }

  void pollEvents() const { fInstance.ProcessEvents(); }

private:
  void asyncInitDevice(std::function<void()> const &onDeviceInitialized,
                       std::function<void(wgpu::StringView)> const &onError);

private:
  wgpu::Instance fInstance{};
  wgpu::Adapter fAdapter{};
  wgpu::Device fDevice{};
  wgpu::Queue fQueue{};
};

class Renderer
{
public:
  explicit Renderer(std::shared_ptr<GPU> iGPU) : fGPU{std::move(iGPU)} {}
  void init();
  void render(int iFrame);

private:

  std::shared_ptr<GPU> fGPU;

  wgpu::RenderPipeline fRenderPipeline{};
  wgpu::Surface fSurface{};
  wgpu::TextureView fCanvasDepthStencilView{};
};

//------------------------------------------------------------------------
// GPU::asyncCreate
//------------------------------------------------------------------------
void GPU::asyncCreate(std::function<void(std::shared_ptr<GPU> iGPU)> onCreated,
                      std::function<void(wgpu::StringView)> const &onError)
{
  printf("Initializing...\n");
  auto gpu = std::make_shared<GPU>(wgpu::CreateInstance());
  gpu->asyncInitDevice([gpu, onCreated = std::move(onCreated)] {
                         onCreated(gpu);
                       },
                       onError);
}

//------------------------------------------------------------------------
// GPU::asyncInitDevice
//------------------------------------------------------------------------
void GPU::asyncInitDevice(std::function<void()> const &onDeviceInitialized,
                          std::function<void(wgpu::StringView)> const &onError)
{
  wgpu::RequestAdapterWebXROptions xrOptions = {};
  wgpu::RequestAdapterOptions options = {};
  options.nextInChain = &xrOptions;

  fInstance.RequestAdapter(&options, wgpu::CallbackMode::AllowSpontaneous,
                           [this, onDeviceInitialized, onError](wgpu::RequestAdapterStatus status,
                                                                wgpu::Adapter ad,
                                                                wgpu::StringView message) {
                             if(status != wgpu::RequestAdapterStatus::Success)
                             {
                               onError(message);
                               return;
                             }
                             fAdapter = std::move(ad);

                             wgpu::Limits limits;
                             wgpu::DeviceDescriptor deviceDescriptor;
                             deviceDescriptor.requiredLimits = &limits;
                             deviceDescriptor.SetUncapturedErrorCallback([](const wgpu::Device &,
                                                                            wgpu::ErrorType errorType,
                                                                            wgpu::StringView message) {
                               printf("UncapturedError (errorType=%d): %.*s\n", errorType, (int) message.length, message.data);
                               terminate("UncapturedError");
                             });
                             deviceDescriptor.SetDeviceLostCallback(wgpu::CallbackMode::AllowProcessEvents,
                                                                    [](const wgpu::Device &,
                                                                       wgpu::DeviceLostReason reason,
                                                                       wgpu::StringView message) {
                                                                      printf("DeviceLost (reason=%d): %.*s\n", reason, (int) message.length,
                                                                             message.data);
                                                                    });

                             wgpu::Device device;
                             fAdapter.RequestDevice(&deviceDescriptor, wgpu::CallbackMode::AllowSpontaneous,
                                                    [this, onDeviceInitialized, onError](wgpu::RequestDeviceStatus status,
                                                                                         wgpu::Device dev,
                                                                                         wgpu::StringView message) {
                                                      if(status != wgpu::RequestDeviceStatus::Success)
                                                      {
                                                        onError(message);
                                                        return;
                                                      }
                                                      fDevice = std::move(dev);
                                                      fQueue = fDevice.GetQueue();
                                                      onDeviceInitialized();
                                                    });
                           });
}

static const char shaderCode[] = R"(
    @vertex
    fn main_v(@builtin(vertex_index) idx: u32) -> @builtin(position) vec4<f32> {
        var pos = array<vec2<f32>, 3>(
            vec2<f32>(0.0, 0.5), vec2<f32>(-0.5, -0.5), vec2<f32>(0.5, -0.5));
        return vec4<f32>(pos[idx], 0.0, 1.0);
    }
    @fragment
    fn main_f() -> @location(0) vec4<f32> {
        return vec4<f32>(0.0, 1.0, 1.0, 1.0);
    }
)";

//------------------------------------------------------------------------
// Renderer::init
//------------------------------------------------------------------------
void Renderer::init()
{
  wgpu::ShaderModule shaderModule{};
  {
    wgpu::ShaderSourceWGSL wgslDesc{};
    wgslDesc.code = shaderCode;

    wgpu::ShaderModuleDescriptor descriptor{};
    descriptor.nextInChain = &wgslDesc;
    shaderModule = fGPU->device().CreateShaderModule(&descriptor);
  }

  {
    wgpu::BindGroupLayoutDescriptor bglDesc{};
    auto bgl = fGPU->device().CreateBindGroupLayout(&bglDesc);
    wgpu::BindGroupDescriptor desc{};
    desc.layout = bgl;
    desc.entryCount = 0;
    desc.entries = nullptr;
    fGPU->device().CreateBindGroup(&desc);
  }

  {
    wgpu::PipelineLayoutDescriptor pl{};
    pl.bindGroupLayoutCount = 0;
    pl.bindGroupLayouts = nullptr;

    wgpu::ColorTargetState colorTargetState{};
    colorTargetState.format = wgpu::TextureFormat::BGRA8Unorm;

    wgpu::FragmentState fragmentState{};
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "main_f";
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTargetState;

    wgpu::DepthStencilState depthStencilState{};
    depthStencilState.format = wgpu::TextureFormat::Depth32Float;
    depthStencilState.depthWriteEnabled = true;
    depthStencilState.depthCompare = wgpu::CompareFunction::Always;

    wgpu::RenderPipelineDescriptor descriptor{};
    descriptor.layout = fGPU->device().CreatePipelineLayout(&pl);
    descriptor.vertex.module = shaderModule;
    descriptor.vertex.entryPoint = "main_v";
    descriptor.fragment = &fragmentState;
    descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    descriptor.depthStencil = &depthStencilState;

    fRenderPipeline = fGPU->device().CreateRenderPipeline(&descriptor);
  }

  {
    wgpu::TextureDescriptor descriptor{};
    descriptor.usage = wgpu::TextureUsage::RenderAttachment;
    descriptor.size = {kWidth, kHeight, 1};
    descriptor.format = wgpu::TextureFormat::Depth32Float;
    fCanvasDepthStencilView = fGPU->device().CreateTexture(&descriptor).CreateView();
  }

  {
    wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc{};
    canvasDesc.selector = "#canvas";

    wgpu::SurfaceDescriptor surfDesc{};
    surfDesc.nextInChain = &canvasDesc;
    fSurface = fGPU->instance().CreateSurface(&surfDesc);

    wgpu::SurfaceColorManagement colorManagement{};
    wgpu::SurfaceConfiguration configuration{};
    configuration.nextInChain = &colorManagement;
    configuration.device = fGPU->device();
    configuration.usage = wgpu::TextureUsage::RenderAttachment;
    configuration.format = wgpu::TextureFormat::BGRA8Unorm;
    configuration.width = kWidth;
    configuration.height = kHeight;
    configuration.alphaMode = wgpu::CompositeAlphaMode::Premultiplied;
    configuration.presentMode = wgpu::PresentMode::Fifo;
    fSurface.Configure(&configuration);
  }

}

//------------------------------------------------------------------------
// Renderer::render
//------------------------------------------------------------------------
void Renderer::render(int iFrame)
{
  fGPU->pollEvents();

  wgpu::SurfaceTexture surfaceTexture;
  fSurface.GetCurrentTexture(&surfaceTexture);
  assert(surfaceTexture.status == wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal);
  wgpu::TextureView backbuffer = surfaceTexture.texture.CreateView();

  wgpu::RenderPassColorAttachment attachment{};
  attachment.view = backbuffer;
  attachment.loadOp = wgpu::LoadOp::Clear;
  attachment.storeOp = wgpu::StoreOp::Store;
  attachment.clearValue = {0.5, 0.5, iFrame / 60.0, 1};

  wgpu::RenderPassDescriptor renderpass{};
  renderpass.colorAttachmentCount = 1;
  renderpass.colorAttachments = &attachment;

  wgpu::RenderPassDepthStencilAttachment depthStencilAttachment = {};
  depthStencilAttachment.view = fCanvasDepthStencilView;
  depthStencilAttachment.depthClearValue = 0;
  depthStencilAttachment.depthLoadOp = wgpu::LoadOp::Clear;
  depthStencilAttachment.depthStoreOp = wgpu::StoreOp::Store;

  renderpass.depthStencilAttachment = &depthStencilAttachment;

  wgpu::CommandBuffer commands;
  {
    wgpu::CommandEncoder encoder = fGPU->device().CreateCommandEncoder();
    {
      wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);
      pass.SetPipeline(fRenderPipeline);
      pass.Draw(3);
      pass.End();
    }
    commands = encoder.Finish();
  }

  fGPU->queue().Submit(1, &commands);
}

static std::unique_ptr<Renderer> kRenderer{};
static int kFrameCount = 0;

//------------------------------------------------------------------------
// MainLoop
//------------------------------------------------------------------------
void MainLoop()
{
  if(kFrameCount < 60)
  {
    kFrameCount++;
    kRenderer->render(kFrameCount);
  }
  else
  {
    emscripten_cancel_main_loop();
    printf("Done \n");
  }
}

//------------------------------------------------------------------------
// main
//------------------------------------------------------------------------
int main()
{
  GPU::asyncCreate([](auto iGPU) {
                     kRenderer = std::make_unique<Renderer>(std::move(iGPU));
                     kRenderer->init();
                     emscripten_set_main_loop(MainLoop, 0, true);
                   }, [](auto iMessage) {
                     printf("Error creating the GPU %.*s\n", static_cast<int>(iMessage.length), iMessage.data);
                     terminate("GPU::asyncCreate");
                   });
}

