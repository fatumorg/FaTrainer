#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <dlfcn.h>
#include <cstdio>
#include <fstream>
#include <string>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_opengl3_loader.h"
#include "ui.h"

static const char* program = "FaTrainer";

extern "C" void* glfwGetX11Display(void);
extern "C" unsigned long glfwGetX11Window(GLFWwindow* window);

static void ask_to_float(GLFWwindow* window) {
    using InternAtom = unsigned long (*)(void*, const char*, int);
    using ChangeProperty = int (*)(void*, unsigned long, unsigned long, unsigned long, int, int, const unsigned char*, int);
    const unsigned long ATOM_TYPE = 4;
    const int FORMAT_32 = 32, REPLACE = 0;
    void* xlib = dlopen("libX11.so.6", RTLD_LAZY | RTLD_NOLOAD);
    if (!xlib) xlib = dlopen("libX11.so.6", RTLD_LAZY);
    auto intern = xlib ? (InternAtom)dlsym(xlib, "XInternAtom") : nullptr;
    auto change = xlib ? (ChangeProperty)dlsym(xlib, "XChangeProperty") : nullptr;
    void* display = glfwGetX11Display();
    if (!intern || !change || !display) return;
    unsigned long type = intern(display, "_NET_WM_WINDOW_TYPE", 0), dialog = intern(display, "_NET_WM_WINDOW_TYPE_DIALOG", 0);
    change(display, glfwGetX11Window(window), type, ATOM_TYPE, FORMAT_32, REPLACE, (const unsigned char*)&dialog, 1);
}

static logic::Area focused_area() {
    logic::Area area;
    if (getenv("HYPRLAND_INSTANCE_SIGNATURE")) {
        std::string text;
        platform::run({"hyprctl", "monitors"}, [&](const char* d, size_t n) { text.append(d, n); return true; });
        area = logic::hyprland_focused_monitor(text);
        if (area.w > 0) return area;
    }
    using DefaultRoot = unsigned long (*)(void*);
    using QueryPointer = int (*)(void*, unsigned long, unsigned long*, unsigned long*, int*, int*, int*, int*, unsigned int*);
    void* xlib = dlopen("libX11.so.6", RTLD_LAZY | RTLD_NOLOAD);
    auto root_of = xlib ? (DefaultRoot)dlsym(xlib, "XDefaultRootWindow") : nullptr;
    auto query = xlib ? (QueryPointer)dlsym(xlib, "XQueryPointer") : nullptr;
    void* display = glfwGetX11Display();
    unsigned long root = 0, child = 0;
    int px = 0, py = 0, wx = 0, wy = 0;
    unsigned int mask = 0;
    bool pointer = root_of && query && display && query(display, root_of(display), &root, &child, &px, &py, &wx, &wy, &mask);
    int count = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    for (int i = 0; i < count; i++) {
        logic::Area a;
        glfwGetMonitorWorkarea(monitors[i], &a.x, &a.y, &a.w, &a.h);
        bool inside = pointer && px >= a.x && px < a.x + a.w && py >= a.y && py < a.y + a.h;
        if (inside || (i == 0 && area.w == 0)) area = a;
        if (inside) break;
    }
    return area;
}

static void center(GLFWwindow* window) {
    logic::Area area = focused_area();
    if (area.w <= 0) return;
    int w = 0, h = 0;
    glfwGetWindowSize(window, &w, &h);
    glfwSetWindowPos(window, area.x + std::max(0, (area.w - w) / 2), area.y + std::max(0, (area.h - h) / 2));
}

static void nixos_hint() {
    if (platform::on_nixos()) fprintf(stderr, "On NixOS, start it through Steam's runtime instead: steam-run %s\n", program);
}

int main(int, char** argv) {
    program = argv[0];
    glfwSetErrorCallback([](int code, const char* text) { fprintf(stderr, "glfw %d: %s\n", code, text); });
    if (!glfwInit()) {
        fprintf(stderr, "FaTrainer needs a desktop session with X11 or XWayland.\n");
        nixos_hint();
        return 1;
    }
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    glfwWindowHintString(GLFW_X11_CLASS_NAME, "fatrainer");
    glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "fatrainer");
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    struct Context { int major, minor; const char* glsl; };
    const Context contexts[] = {{3, 0, "#version 130"}, {2, 1, "#version 120"}};
    GLFWwindow* window = nullptr;
    const char* glsl = nullptr;
    for (const Context& c : contexts) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, c.major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, c.minor);
        if ((window = glfwCreateWindow(1120, 740, "FaTrainer (" FATRAINER_EDITION ")", nullptr, nullptr))) {
            glsl = c.glsl;
            break;
        }
    }
    if (!window) {
        fprintf(stderr, "FaTrainer could not open an OpenGL window.\n");
        nixos_hint();
        glfwTerminate();
        return 1;
    }
    glfwSetWindowSizeLimits(window, 900, 620, GLFW_DONT_CARE, GLFW_DONT_CARE);
    ask_to_float(window);
    center(window);
    glfwShowWindow(window);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    float scale_x = 1, scale_y = 1;
    glfwGetWindowContentScale(window, &scale_x, &scale_y);
    ImGui::CreateContext();
    app::init(std::max(1.0f, scale_x));
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl);
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
            glfwWaitEventsTimeout(0.2);
            continue;
        }
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        app::frame();
        ImGui::Render();
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(17 / 255.0f, 17 / 255.0f, 19 / 255.0f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
