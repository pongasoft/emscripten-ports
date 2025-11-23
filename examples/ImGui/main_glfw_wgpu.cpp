// Dear ImGui: standalone example application for GLFW + WebGPU
// - Emscripten is supported for publishing on web. See https://emscripten.org.
// - Dawn is used as a WebGPU implementation on desktop.

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_wgpu.h>
#include <stdio.h>
#include <emscripten/version.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLFW/emscripten_glfw3.h>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>
#include <functional>

// Global WebGPU required states
static WGPUInstance wgpu_instance = nullptr;
static WGPUDevice wgpu_device = nullptr;
static WGPUSurface wgpu_surface = nullptr;
static WGPUQueue wgpu_queue = nullptr;
static WGPUSurfaceConfiguration wgpu_surface_configuration = {};
static int wgpu_surface_width = 1280;
static int wgpu_surface_height = 800;

// Forward declarations
static bool InitWGPU();

static WGPUSurface CreateWGPUSurface(const WGPUInstance &instance, GLFWwindow *window);

static void glfw_error_callback(int error, const char *description)
{
  printf("GLFW Error %d: %s\n", error, description);
}

static void ResizeSurface(int width, int height)
{
  wgpu_surface_configuration.width = wgpu_surface_width = width;
  wgpu_surface_configuration.height = wgpu_surface_height = height;
  wgpuSurfaceConfigure(wgpu_surface, &wgpu_surface_configuration);
}

struct App
{
  std::function<bool()> renderFrame{};
  std::function<void()> cleanup{};
};

static void MainLoopForEmscripten(void *iUserData)
{
  auto app = reinterpret_cast<App *>(iUserData);
  if(app->renderFrame())
  {
    if(app->cleanup)
      app->cleanup();
    emscripten_cancel_main_loop();
  }
}

// Main code
int main(int, char **)
{
  glfwSetErrorCallback(glfw_error_callback);
  if(!glfwInit())
    return 1;

  printf("Emscripten: %d.%d.%d\n", __EMSCRIPTEN_major__, __EMSCRIPTEN_minor__, __EMSCRIPTEN_tiny__);
  printf("GLFW: %s\n", glfwGetVersionString());
  printf("ImGui: %s\n", IMGUI_VERSION);

  // Make sure GLFW does not initialize any graphics context.
  // This needs to be done explicitly later.
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only

  GLFWwindow *window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+WebGPU example", nullptr, nullptr);
  if(window == nullptr)
    return 1;

  // Initialize the WebGPU environment
  if(!InitWGPU())
  {
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }
  glfwShowWindow(window);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void) io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

#ifdef IMGUI_ENABLE_DOCKING
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigDockingWithShift = false;
#endif

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsLight();

  // Setup scaling
  ImGuiStyle &style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
  style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOther(window, true);
  // makes the canvas resizable and match the full window size
  emscripten_glfw_make_canvas_resizable(window, "window", nullptr);
  ImGui_ImplWGPU_InitInfo init_info;
  init_info.Device = wgpu_device;
  init_info.NumFramesInFlight = 3;
  init_info.RenderTargetFormat = wgpu_surface_configuration.format;
  init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;
  ImGui_ImplWGPU_Init(&init_info);

  // Our state
  bool show_demo_window = true;
  bool show_another_window = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
  // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
  io.IniFilename = nullptr;

  // Main loop
  App app{};
  app.renderFrame = [&]() {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();

    // React to changes in screen size
    int width, height;
    glfwGetFramebufferSize((GLFWwindow *) window, &width, &height);
    if(width != wgpu_surface_width || height != wgpu_surface_height)
      ResizeSurface(width, height);

    // Check surface status for error. If texture is not optimal, try to reconfigure the surface.
    WGPUSurfaceTexture surface_texture;
    wgpuSurfaceGetCurrentTexture(wgpu_surface, &surface_texture);
    if(ImGui_ImplWGPU_IsSurfaceStatusError(surface_texture.status))
    {
      fprintf((stderr), "Unrecoverable Surface Texture status=%#.8x\n", surface_texture.status);
      abort();
    }
    if(ImGui_ImplWGPU_IsSurfaceStatusSubOptimal(surface_texture.status))
    {
      if(surface_texture.texture)
        wgpuTextureRelease(surface_texture.texture);
      if(width > 0 && height > 0)
        ResizeSurface(width, height);
      return false;
    }

    // Start the Dear ImGui frame
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

#ifdef IMGUI_ENABLE_DOCKING
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);
#endif

#ifndef IMGUI_DISABLE_DEMO
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if(show_demo_window)
      ImGui::ShowDemoWindow(&show_demo_window);
#endif

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
      static float f = 0.0f;
      static int counter = 0;

      ImGui::Begin("Hello, world!");                                // Create a window called "Hello, world!" and append into it.

      ImGui::Text("This is some useful text.");                     // Display some text (you can use a format strings too)
      ImGui::Checkbox("Demo Window", &show_demo_window);            // Edit bools storing our window open/close state
      ImGui::Checkbox("Another Window", &show_another_window);

      ImGui::SliderFloat("float", &f, 0.0f, 1.0f);                  // Edit 1 float using a slider from 0.0f to 1.0f
      ImGui::ColorEdit3("clear color", (float *) &clear_color);     // Edit 3 floats representing a color

      if(ImGui::Button("Button"))                                  // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
      ImGui::SameLine();
      ImGui::Text("counter = %d", counter);

      if(ImGui::Button("Exit"))
        glfwSetWindowShouldClose(window, GLFW_TRUE);

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
      ImGui::End();
    }

    // 3. Show another simple window.
    if(show_another_window)
    {
      ImGui::Begin("Another Window",
                   &show_another_window);         // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
      ImGui::Text("Hello from another window!");
      if(ImGui::Button("Close Me"))
        show_another_window = false;
      ImGui::End();
    }

    // Rendering
    ImGui::Render();

    WGPUTextureViewDescriptor view_desc = {};
    view_desc.format = wgpu_surface_configuration.format;
    view_desc.dimension = WGPUTextureViewDimension_2D;
    view_desc.mipLevelCount = WGPU_MIP_LEVEL_COUNT_UNDEFINED;
    view_desc.arrayLayerCount = WGPU_ARRAY_LAYER_COUNT_UNDEFINED;
    view_desc.aspect = WGPUTextureAspect_All;

    WGPUTextureView texture_view = wgpuTextureCreateView(surface_texture.texture, &view_desc);

    WGPURenderPassColorAttachment color_attachments = {};
    color_attachments.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    color_attachments.loadOp = WGPULoadOp_Clear;
    color_attachments.storeOp = WGPUStoreOp_Store;
    color_attachments.clearValue = {clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                                    clear_color.z * clear_color.w, clear_color.w};
    color_attachments.view = texture_view;

    WGPURenderPassDescriptor render_pass_desc = {};
    render_pass_desc.colorAttachmentCount = 1;
    render_pass_desc.colorAttachments = &color_attachments;
    render_pass_desc.depthStencilAttachment = nullptr;

    WGPUCommandEncoderDescriptor enc_desc = {};
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(wgpu_device, &enc_desc);

    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);
    wgpuRenderPassEncoderEnd(pass);

    WGPUCommandBufferDescriptor cmd_buffer_desc = {};
    WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
    wgpuQueueSubmit(wgpu_queue, 1, &cmd_buffer);

    wgpuTextureViewRelease(texture_view);
    wgpuRenderPassEncoderRelease(pass);
    wgpuCommandEncoderRelease(encoder);
    wgpuCommandBufferRelease(cmd_buffer);

    return glfwWindowShouldClose(window) == GLFW_TRUE;
  };

  app.cleanup = [window]() {
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    wgpuSurfaceUnconfigure(wgpu_surface);
    wgpuSurfaceRelease(wgpu_surface);
    wgpuQueueRelease(wgpu_queue);
    wgpuDeviceRelease(wgpu_device);
    wgpuInstanceRelease(wgpu_instance);

    glfwDestroyWindow(window);
    glfwTerminate();
  };

  emscripten_set_main_loop_arg(MainLoopForEmscripten, &app, 0, true);

  return 0;
}

static WGPUAdapter RequestAdapter(wgpu::Instance &instance)
{
  wgpu::Adapter acquired_adapter;
  wgpu::RequestAdapterOptions adapter_options;
  auto onRequestAdapter = [&](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) {
    if(status != wgpu::RequestAdapterStatus::Success)
    {
      printf("Failed to get an adapter: %s\n", message.data);
      return;
    }
    acquired_adapter = std::move(adapter);
  };

  wgpu::Future waitAdapterFunc { instance.RequestAdapter(&adapter_options, wgpu::CallbackMode::WaitAnyOnly, onRequestAdapter) };
  // This synchronous call requires the "-s ASYNCIFY=1" option when compiling this example
  wgpu::WaitStatus waitStatusAdapter = instance.WaitAny(waitAdapterFunc, UINT64_MAX);
  IM_ASSERT(acquired_adapter != nullptr && waitStatusAdapter == wgpu::WaitStatus::Success && "Error on Adapter request");
  return acquired_adapter.MoveToCHandle();
}

static WGPUDevice RequestDevice(wgpu::Instance& instance, wgpu::Adapter& adapter)
{
  // Set device callback functions
  wgpu::DeviceDescriptor device_desc;
  device_desc.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous,
                                    [](const wgpu::Device&, wgpu::DeviceLostReason type, wgpu::StringView msg) {
                                      fprintf(stderr, "%s error: %s\n", ImGui_ImplWGPU_GetDeviceLostReasonName((WGPUDeviceLostReason)type), msg.data);
                                    }
  );
  device_desc.SetUncapturedErrorCallback([](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView msg) {
    fprintf(stderr, "%s error: %s\n", ImGui_ImplWGPU_GetErrorTypeName((WGPUErrorType)type), msg.data); }
  );

  wgpu::Device acquired_device;
  auto onRequestDevice = [&](wgpu::RequestDeviceStatus status, wgpu::Device local_device, wgpu::StringView message) {
    if (status != wgpu::RequestDeviceStatus::Success)
    {
      printf("Failed to get an device: %s\n", message.data);
      return;
    }
    acquired_device = std::move(local_device);
  };

  // Synchronously (wait until) get Device
  wgpu::Future waitDeviceFunc { adapter.RequestDevice(&device_desc, wgpu::CallbackMode::WaitAnyOnly, onRequestDevice) };
  // This synchronous call requires the "-s ASYNCIFY=1" option when compiling this example
  wgpu::WaitStatus waitStatusDevice = instance.WaitAny(waitDeviceFunc, UINT64_MAX);
  IM_ASSERT(acquired_device != nullptr && waitStatusDevice == wgpu::WaitStatus::Success && "Error on Device request");
  return acquired_device.MoveToCHandle();
}

static bool InitWGPU()
{
  WGPUTextureFormat preferred_fmt = WGPUTextureFormat_Undefined;

  wgpu::InstanceDescriptor instance_desc = {};
  static constexpr wgpu::InstanceFeatureName timedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
  instance_desc.requiredFeatureCount = 1;
  instance_desc.requiredFeatures = &timedWaitAny;
  wgpu::Instance instance = wgpu::CreateInstance(&instance_desc);

  wgpu::Adapter adapter = RequestAdapter(instance);
  ImGui_ImplWGPU_DebugPrintAdapterInfo(adapter.Get());

  wgpu_device = RequestDevice(instance, adapter);

  wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvas_desc = {};
  canvas_desc.selector = "#canvas";

  wgpu::SurfaceDescriptor surface_desc = {};
  surface_desc.nextInChain = &canvas_desc;
  wgpu_surface = instance.CreateSurface(&surface_desc).MoveToCHandle();

  if(!wgpu_surface)
    return false;

  wgpu_instance = instance.MoveToCHandle();

  WGPUSurfaceCapabilities surface_capabilities = {};
  wgpuSurfaceGetCapabilities(wgpu_surface, adapter.Get(), &surface_capabilities);

  preferred_fmt = surface_capabilities.formats[0];

  wgpu_surface_configuration.presentMode = WGPUPresentMode_Fifo;
  wgpu_surface_configuration.alphaMode = WGPUCompositeAlphaMode_Auto;
  wgpu_surface_configuration.usage = WGPUTextureUsage_RenderAttachment;
  wgpu_surface_configuration.width = wgpu_surface_width;
  wgpu_surface_configuration.height = wgpu_surface_height;
  wgpu_surface_configuration.device = wgpu_device;
  wgpu_surface_configuration.format = preferred_fmt;

  wgpuSurfaceConfigure(wgpu_surface, &wgpu_surface_configuration);
  wgpu_queue = wgpuDeviceGetQueue(wgpu_device);

  return true;
}
