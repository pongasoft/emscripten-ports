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
#include <emscripten.h>
#include <emscripten/html5.h>

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

  static std::unique_ptr<GPU> create();

  inline wgpu::WaitStatus wait(wgpu::Future f) const { return fInstance.WaitAny(f, UINT64_MAX); }

  wgpu::Instance &instance() { return fInstance; }
  wgpu::Device &device() { return fDevice; }
  wgpu::Queue &queue() { return fQueue; }

private:
  void initDevice();

private:
  wgpu::Instance fInstance{};
  wgpu::Device fDevice{};
  wgpu::Queue fQueue{};
};

class Renderer
{
public:
  void init(GPU &iGPU);
  void render(GPU &iGPU, int iFrame);

private:
  wgpu::RenderPipeline fRenderPipeline{};
  wgpu::Surface fSurface{};
  wgpu::TextureView fCanvasDepthStencilView{};
};

std::unique_ptr<GPU> GPU::create()
{
  printf("Initializing...\n");
  wgpu::InstanceDescriptor instanceDescriptor;
  instanceDescriptor.capabilities.timedWaitAnyEnable = true;
  std::unique_ptr<GPU> instance = std::make_unique<GPU>(wgpu::CreateInstance(&instanceDescriptor));
  if(!instance->fInstance)
    terminate("Cannot create instance");
  instance->initDevice();
  return instance;
}

void GPU::initDevice()
{
  wgpu::Limits limits;
  wgpu::DeviceDescriptor deviceDescriptor;
  deviceDescriptor.requiredLimits = &limits;
  deviceDescriptor.SetUncapturedErrorCallback(
    [](const wgpu::Device &, wgpu::ErrorType errorType, wgpu::StringView message) {
      printf("UncapturedError (errorType=%d): %.*s\n", errorType, (int) message.length, message.data);
      assert(false);
    });
  deviceDescriptor.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous,
                                         [](const wgpu::Device &, wgpu::DeviceLostReason reason,
                                            wgpu::StringView message) {
                                           printf("DeviceLost (reason=%d): %.*s\n", reason, (int) message.length, message.data);
                                         });

  wgpu::RequestAdapterWebXROptions xrOptions = {};
  wgpu::RequestAdapterOptions options = {};
  options.nextInChain = &xrOptions;

  wgpu::Adapter adapter;
  wgpu::Future f1 = fInstance.RequestAdapter(&options, wgpu::CallbackMode::WaitAnyOnly,
                                             [&adapter](wgpu::RequestAdapterStatus status, wgpu::Adapter ad,
                                                        wgpu::StringView message) {
                                              if(message.length)
                                              {
                                                printf("RequestAdapter: %.*s\n", (int) message.length, message.data);
                                              }
                                               if(status == wgpu::RequestAdapterStatus::Unavailable)
                                               {
                                                 printf("WebGPU unavailable; exiting cleanly\n");
                                                 // exit(0) (rather than emscripten_force_exit(0)) ensures there is no dangling keepalive.
                                                 exit(0);
                                               }
                                               assert(status == wgpu::RequestAdapterStatus::Success);
                                               adapter = std::move(ad);
                                             });
  wait(f1);
  assert(adapter);

  wgpu::Device device;
  wgpu::Future f2 = adapter.RequestDevice(&deviceDescriptor, wgpu::CallbackMode::WaitAnyOnly,
                                          [&device](wgpu::RequestDeviceStatus status, wgpu::Device dev,
                                                    wgpu::StringView message) {
                                            if(message.length)
                                            {
                                              printf("RequestDevice: %.*s\n", (int) message.length, message.data);
                                            }
                                            assert(status == wgpu::RequestDeviceStatus::Success);

                                            device = std::move(dev);
                                          });
  wait(f2);
  assert(device);

  fDevice = std::move(device);

  fQueue = fDevice.GetQueue();
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

void Renderer::init(GPU &iGPU)
{
  wgpu::ShaderModule shaderModule{};
  {
    wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
    wgslDesc.code = shaderCode;

    wgpu::ShaderModuleDescriptor descriptor{};
    descriptor.nextInChain = &wgslDesc;
    shaderModule = iGPU.device().CreateShaderModule(&descriptor);
  }

  {
    wgpu::BindGroupLayoutDescriptor bglDesc{};
    auto bgl = iGPU.device().CreateBindGroupLayout(&bglDesc);
    wgpu::BindGroupDescriptor desc{};
    desc.layout = bgl;
    desc.entryCount = 0;
    desc.entries = nullptr;
    iGPU.device().CreateBindGroup(&desc);
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
    descriptor.layout = iGPU.device().CreatePipelineLayout(&pl);
    descriptor.vertex.module = shaderModule;
    descriptor.vertex.entryPoint = "main_v";
    descriptor.fragment = &fragmentState;
    descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    descriptor.depthStencil = &depthStencilState;

    wgpu::Future f = iGPU.device().CreateRenderPipelineAsync(&descriptor, wgpu::CallbackMode::WaitAnyOnly,
                                                              [this](wgpu::CreatePipelineAsyncStatus status,
                                                                     wgpu::RenderPipeline pl, wgpu::StringView message) {
                                                                if(message.length)
                                                                {
                                                                  printf("CreateRenderPipelineAsync: %.*s\n", (int) message.length, message.data);
                                                                }
                                                                assert(status == wgpu::CreatePipelineAsyncStatus::Success);
                                                                fRenderPipeline = std::move(pl);
                                                              });
    iGPU.wait(f);
    assert(fRenderPipeline);
  }

  {
    wgpu::TextureDescriptor descriptor{};
    descriptor.usage = wgpu::TextureUsage::RenderAttachment;
    descriptor.size = {kWidth, kHeight, 1};
    descriptor.format = wgpu::TextureFormat::Depth32Float;
    fCanvasDepthStencilView = iGPU.device().CreateTexture(&descriptor).CreateView();
  }

  {
    wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc{};
    canvasDesc.selector = "#canvas";

    wgpu::SurfaceDescriptor surfDesc{};
    surfDesc.nextInChain = &canvasDesc;
    fSurface = iGPU.instance().CreateSurface(&surfDesc);

    wgpu::SurfaceColorManagement colorManagement{};
    wgpu::SurfaceConfiguration configuration{};
    configuration.nextInChain = &colorManagement;
    configuration.device = iGPU.device();
    configuration.usage = wgpu::TextureUsage::RenderAttachment;
    configuration.format = wgpu::TextureFormat::BGRA8Unorm;
    configuration.width = kWidth;
    configuration.height = kHeight;
    configuration.alphaMode = wgpu::CompositeAlphaMode::Premultiplied;
    configuration.presentMode = wgpu::PresentMode::Fifo;
    fSurface.Configure(&configuration);
  }

}

void Renderer::render(GPU &iGPU, int iFrame)
{
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
    wgpu::CommandEncoder encoder = iGPU.device().CreateCommandEncoder();
    {
      wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);
      pass.SetPipeline(fRenderPipeline);
      pass.Draw(3);
      pass.End();
    }
    commands = encoder.Finish();
  }

  iGPU.queue().Submit(1, &commands);
}

int main()
{
  auto gpu = GPU::create();
  
  auto renderer = std::make_unique<Renderer>();

  renderer->init(*gpu);

  int frameCount = 0;

  while(frameCount < 60)
  {
    frameCount++;
    renderer->render(*gpu, frameCount);
    emscripten_sleep(16);
  }

  printf("Done \n");
}

