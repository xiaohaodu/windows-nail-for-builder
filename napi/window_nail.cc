#include <napi.h>
#include <iostream>
#include <vector>
#include <string>

// 跨平台头文件
#ifdef _WIN32
    #include <windows.h>
    #include <tchar.h>
#elif __APPLE__
    #include <CoreGraphics/CoreGraphics.h>
    #include <ApplicationServices/ApplicationServices.h>
    #include <Carbon/Carbon.h>
    #include <mach/mach.h>
#endif

#include "third_party/WINDOWINFO/WINDOWINFO.h"
#include "third_party/LOG/LOG.h"

#ifdef _WIN32
typedef HWND WindowHandle;
bool IsSystemWindowClass(WindowHandle hwnd)
{
    TCHAR className[256];
    GetClassName(hwnd, className, sizeof(className));

    static const TCHAR *SYSTEM_WINDOW_CLASS_NAMES[] = {
        _T("Progman"),
        _T("Shell_TrayWnd"),
        _T("WorkerW"),
        _T("Windows.UI.Core.CoreWindow"),
        _T("ApplicationFrameWindow"),
        _T("Windows.Internal.Shell.TabProxyWindow"),
        _T("Xaml_WindowedPopupClass"),
        _T("HwndWrapper")
    };

    for (const auto &systemClassName : SYSTEM_WINDOW_CLASS_NAMES)
    {
        if (_tcsncmp(className, systemClassName, _tcslen(systemClassName)) == 0)
        {
            return true;
        }
    }
    return false;
}
#elif __APPLE__
typedef CGWindowID WindowHandle;
bool IsSystemWindowClass(WindowHandle windowId)
{
    // macOS 系统窗口检查
    CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionAll, kCGNullWindowID);
    if (!windowList)
        return false;

    CFDictionaryRef windowInfo = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windowList, 0));
    if (!windowInfo)
    {
        CFRelease(windowList);
        return false;
    }

    CFStringRef windowName = static_cast<CFStringRef>(CFDictionaryGetValue(windowInfo, kCGWindowName));
    CFStringRef ownerName = static_cast<CFStringRef>(CFDictionaryGetValue(windowInfo, kCGWindowOwnerName));

    // 检查系统进程
    if (ownerName)
    {
        char ownerNameC[256];
        CFStringGetCString(ownerName, ownerNameC, sizeof(ownerNameC));
        std::string ownerNameStr(ownerNameC);
        
        // 系统进程列表
        std::vector<std::string> systemProcesses = {
            "SystemUIServer",
            "WindowServer",
            "Dock",
            "Finder"
        };
        
        for (const auto &process : systemProcesses)
        {
            if (ownerNameStr == process)
            {
                CFRelease(windowList);
                return true;
            }
        }
    }

    CFRelease(windowList);
    return false;
}
#endif

#ifdef _WIN32
struct EnumData
{
    POINT point;
    HWND hwndResult;
    EnumData(int x, int y) : point{x, y}, hwndResult{nullptr} {} 
};
#elif __APPLE__
struct EnumData
{
    CGPoint point;
    CGWindowID windowIdResult;
    EnumData(int x, int y) : point{x, y}, windowIdResult(0) {} 
};
#endif

Napi::Value SwitchWindowTopmostByTitle(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsBoolean())
    {
        Napi::TypeError::New(env, "First argument must be a window title string, and second argument must be a boolean indicating topmost status").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    std::string windowTitle = info[0].As<Napi::String>().Utf8Value();
    bool setToTopmost = info[1].As<Napi::Boolean>().Value();

#ifdef _WIN32
    // Windows 实现
    std::wstring windowTitleW(windowTitle.begin(), windowTitle.end());
    HWND hWndTarget = FindWindowW(NULL, windowTitleW.c_str());
    if (hWndTarget == NULL)
    {
        Napi::Error::New(env, "Failed to find the target window").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    HWND insertAfter = setToTopmost ? HWND_TOPMOST : HWND_NOTOPMOST;

    // 设置窗口层级
    if (!SetWindowPos(hWndTarget, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE))
    {
        std::string errorMsg = setToTopmost ? "Failed to set the window as topmost" : "Failed to set the window as normal";
        Napi::Error::New(env, errorMsg).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

#elif __APPLE__
    // macOS 实现
    CFStringRef windowTitleCF = CFStringCreateWithCString(NULL, windowTitle.c_str(), kCFStringEncodingUTF8);
    if (!windowTitleCF)
    {
        Napi::Error::New(env, "Failed to create CFString").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    bool found = false;
    CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionAll, kCGNullWindowID);
    if (windowList)
    {
        CFIndex count = CFArrayGetCount(windowList);
        for (CFIndex i = 0; i < count; i++)
        {
            CFDictionaryRef windowInfo = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windowList, i));
            CFStringRef windowName = static_cast<CFStringRef>(CFDictionaryGetValue(windowInfo, kCGWindowName));
            
            if (windowName && CFStringCompare(windowName, windowTitleCF, 0) == kCFCompareEqualTo)
            {
                // 找到窗口，获取窗口ID
                CFNumberRef windowIdRef = static_cast<CFNumberRef>(CFDictionaryGetValue(windowInfo, kCGWindowNumber));
                if (windowIdRef)
                {
                    CGWindowID windowId;
                    CFNumberGetValue(windowIdRef, kCFNumberSInt32Type, &windowId);
                    
                    // 在 macOS 中，我们需要使用 AppleScript 或其他方法来设置窗口置顶
                    // 这里使用 AppleScript 来实现
                    std::string script = setToTopmost 
                        ? "tell application \"System Events\" to set frontmost of every process whose unix id is " + std::to_string(getpid()) + " to true"
                        : "tell application \"System Events\" to set frontmost of every process whose unix id is " + std::to_string(getpid()) + " to false";
                    
                    system(script.c_str());
                    found = true;
                    break;
                }
            }
        }
        CFRelease(windowList);
    }
    CFRelease(windowTitleCF);

    if (!found)
    {
        Napi::Error::New(env, "Failed to find the target window").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

#endif

    std::string logMsg = setToTopmost ? "set window top" : "set window top normal";
    Log::LogMessage(env, logMsg);

    return Napi::Boolean::New(env, true);
}

Napi::Value SwitchWindowTopmostByHWND(const Napi::CallbackInfo &info)
{
    // 获取当前环境
    Napi::Env env = info.Env();

    // 检查参数数量和类型
    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsBoolean())
    {
        // 参数错误，抛出异常并返回false
        Napi::TypeError::New(env, "First argument must be a window handle (as an integer), and second argument must be a boolean indicating topmost status").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    // 获取窗口句柄
    uint32_t windowHandleValue = info[0].As<Napi::Number>().Uint32Value();
    bool setToTopmost = info[1].As<Napi::Boolean>().Value();

#ifdef _WIN32
    // Windows 实现
    HWND hWndTarget = reinterpret_cast<HWND>(static_cast<uintptr_t>(windowHandleValue));

    // 检查窗口句柄有效性
    if (!IsWindow(hWndTarget)) // 使用Windows API IsWindow检查窗口句柄是否有效
    {
        // 窗口句柄无效，抛出异常并返回false
        Napi::Error::New(env, "Invalid window handle provided").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    HWND insertAfter = setToTopmost ? HWND_TOPMOST : HWND_NOTOPMOST;

    // 设置窗口层级
    if (!SetWindowPos(hWndTarget, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE))
    {
        std::string errorMsg = setToTopmost ? "Failed to set the window as topmost" : "Failed to set the window as normal";
        // 设置窗口层级失败，抛出异常并返回false
        Napi::Error::New(env, errorMsg).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

#elif __APPLE__
    // macOS 实现
    CGWindowID windowId = static_cast<CGWindowID>(windowHandleValue);
    
    // 检查窗口ID有效性
    bool windowExists = false;
    CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionAll, kCGNullWindowID);
    if (windowList)
    {
        CFIndex count = CFArrayGetCount(windowList);
        for (CFIndex i = 0; i < count; i++)
        {
            CFDictionaryRef windowInfo = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windowList, i));
            CFNumberRef windowIdRef = static_cast<CFNumberRef>(CFDictionaryGetValue(windowInfo, kCGWindowNumber));
            if (windowIdRef)
            {
                CGWindowID currentWindowId;
                CFNumberGetValue(windowIdRef, kCFNumberSInt32Type, &currentWindowId);
                if (currentWindowId == windowId)
                {
                    windowExists = true;
                    break;
                }
            }
        }
        CFRelease(windowList);
    }

    if (!windowExists)
    {
        Napi::Error::New(env, "Invalid window handle provided").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    // 在 macOS 中，我们需要使用 AppleScript 或其他方法来设置窗口置顶
    std::string script = setToTopmost 
        ? "tell application \"System Events\" to set frontmost of every process whose unix id is " + std::to_string(getpid()) + " to true"
        : "tell application \"System Events\" to set frontmost of every process whose unix id is " + std::to_string(getpid()) + " to false";
    
    system(script.c_str());

#endif

    // 记录操作结果
    std::string logMsg = setToTopmost ? "Successfully set window to topmost" : "Successfully set window to normal";
    Log::LogMessage(env, logMsg);

    // 成功执行，返回true
    return Napi::Boolean::New(env, true);
}

#ifdef _WIN32
std::wstring GetWindowTitle(HWND hWnd)
{
    wchar_t titleBuffer[512];
    int length = GetWindowTextW(hWnd, titleBuffer, ARRAYSIZE(titleBuffer));
    return std::wstring(titleBuffer, length);
}
#elif __APPLE__
std::string GetWindowTitle(CGWindowID windowId)
{
    CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionAll, kCGNullWindowID);
    if (!windowList)
        return "";

    std::string title;
    CFIndex count = CFArrayGetCount(windowList);
    for (CFIndex i = 0; i < count; i++)
    {
        CFDictionaryRef windowInfo = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windowList, i));
        CFNumberRef windowIdRef = static_cast<CFNumberRef>(CFDictionaryGetValue(windowInfo, kCGWindowNumber));
        if (windowIdRef)
        {
            CGWindowID currentWindowId;
            CFNumberGetValue(windowIdRef, kCFNumberSInt32Type, &currentWindowId);
            if (currentWindowId == windowId)
            {
                CFStringRef windowName = static_cast<CFStringRef>(CFDictionaryGetValue(windowInfo, kCGWindowName));
                if (windowName)
                {
                    char titleBuffer[512];
                    CFStringGetCString(windowName, titleBuffer, sizeof(titleBuffer));
                    title = titleBuffer;
                }
                break;
            }
        }
    }

    CFRelease(windowList);
    return title;
}
#endif

#ifdef _WIN32
static BOOL CALLBACK CheckWindow(HWND hwnd, LPARAM lParam)
{
    EnumData *pData = reinterpret_cast<EnumData *>(lParam);
    HWND *foregroundHwnd = &pData->hwndResult;
    POINT *point = &pData->point;
    RECT rect;

    if (!GetWindowRect(hwnd, &rect))
        return TRUE;

    bool isSystemWindow = IsSystemWindowClass(hwnd);

    if (isSystemWindow || !IsWindowVisible(hwnd) || (GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE) == 0)
        return TRUE;

    // 检查点是否位于窗口内
    if (PtInRect(&rect, *point))
    {
        // 检查窗口是否有标题（可选，根据具体需求）
        std::wstring windowTitle = GetWindowTitle(hwnd);
        if (!windowTitle.empty())
        {
            *foregroundHwnd = hwnd;
            return FALSE; // 找到匹配的窗口，停止枚举
        }
    }

    return TRUE;
}

Napi::Value GetForegroundWindowAtPosition(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 2 || !info[0].IsNumber() || !info[1].IsNumber())
    {
        Napi::TypeError::New(env, "Expected two number arguments: x and y").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    int x = info[0].As<Napi::Number>().Int32Value();
    int y = info[1].As<Napi::Number>().Int32Value();

    EnumData data(x, y); // 使用构造函数初始化
    EnumWindows(CheckWindow, (LPARAM)&data);

    if (data.hwndResult == nullptr)
    {
        return env.Null(); // 没有找到符合条件的窗口
    }

    return WindowInfo(env, data.hwndResult).ToObject(env);
}
#elif __APPLE__
Napi::Value GetForegroundWindowAtPosition(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 2 || !info[0].IsNumber() || !info[1].IsNumber())
    {
        Napi::TypeError::New(env, "Expected two number arguments: x and y").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    int x = info[0].As<Napi::Number>().Int32Value();
    int y = info[1].As<Napi::Number>().Int32Value();

    // macOS 实现：获取指定位置的窗口
    CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionAll, kCGNullWindowID);
    if (!windowList)
        return env.Null();

    CGWindowID targetWindowId = 0;
    CFIndex count = CFArrayGetCount(windowList);
    
    // 从顶层窗口开始检查
    for (CFIndex i = 0; i < count; i++)
    {
        CFDictionaryRef windowInfo = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windowList, i));
        
        // 获取窗口位置和大小
        CFDictionaryRef boundsDict = static_cast<CFDictionaryRef>(CFDictionaryGetValue(windowInfo, kCGWindowBounds));
        if (!boundsDict)
            continue;

        CGRect bounds;
        CGRectMakeWithDictionaryRepresentation(boundsDict, &bounds);
        
        // 检查点是否在窗口内
        if (x >= bounds.origin.x && x <= bounds.origin.x + bounds.size.width &&
            y >= bounds.origin.y && y <= bounds.origin.y + bounds.size.height)
        {
            // 检查是否是系统窗口
            CFStringRef ownerName = static_cast<CFStringRef>(CFDictionaryGetValue(windowInfo, kCGWindowOwnerName));
            if (ownerName)
            {
                char ownerNameC[256];
                CFStringGetCString(ownerName, ownerNameC, sizeof(ownerNameC));
                std::string ownerNameStr(ownerNameC);
                
                // 跳过系统进程
                std::vector<std::string> systemProcesses = {
                    "SystemUIServer",
                    "WindowServer",
                    "Dock",
                    "Finder"
                };
                
                bool isSystemProcess = false;
                for (const auto &process : systemProcesses)
                {
                    if (ownerNameStr == process)
                    {
                        isSystemProcess = true;
                        break;
                    }
                }
                
                if (!isSystemProcess)
                {
                    // 获取窗口ID
                    CFNumberRef windowIdRef = static_cast<CFNumberRef>(CFDictionaryGetValue(windowInfo, kCGWindowNumber));
                    if (windowIdRef)
                    {
                        CFNumberGetValue(windowIdRef, kCFNumberSInt32Type, &targetWindowId);
                        break;
                    }
                }
            }
        }
    }

    CFRelease(windowList);

    if (targetWindowId == 0)
    {
        return env.Null(); // 没有找到符合条件的窗口
    }

    // TODO: 实现 macOS 版本的 WindowInfo
    // 这里暂时返回一个包含窗口ID的对象
    Napi::Object result = Napi::Object::New(env);
    result.Set("windowId", Napi::Number::New(env, targetWindowId));
    return result;
}
#endif

#ifdef _WIN32
struct EnvAndWindowInfos
{
    Napi::Env env;
    std::vector<WindowInfo> &windowInfos;
};
// 自由函数作为 EnumWindows 回调，现在它接收额外的参数用于传递 windowInfos
BOOL CALLBACK EnumWindowProc(HWND hwnd, LPARAM lParam)
{
    EnvAndWindowInfos *data = reinterpret_cast<EnvAndWindowInfos *>(lParam);
    Napi::Env &env = data->env;
    std::vector<WindowInfo> &windowInfos = data->windowInfos;

    bool isSystemWindow = IsSystemWindowClass(hwnd);

    WINDOWPLACEMENT wp;
    ZeroMemory(&wp, sizeof(wp));
    wp.length = sizeof(wp);
    if (!isSystemWindow && GetWindowPlacement(hwnd, &wp) && wp.showCmd != SW_MINIMIZE && (GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE) != 0 && IsWindowVisible(hwnd))
    {
        std::wstring windowTitle = GetWindowTitle(hwnd);
        if (!windowTitle.empty())
        {
            try
            {
                windowInfos.push_back(WindowInfo(env, hwnd));
                // std::cout << "class " << className << " title " << WindowInfo::WideStringToUtf8(windowTitle) << " length " << windowInfos.size() << std::endl;
            }
            catch (const std::exception &e)
            {
                std::cerr << "Failed to create and add WindowInfo: " << e.what() << std::endl;
            }
        }
    }

    // 继续枚举所有窗口
    return TRUE;
}

Napi::Value GetAllWindowsInfo(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    std::vector<WindowInfo> windowInfos;

    EnvAndWindowInfos data{env, windowInfos};

    EnumWindows(EnumWindowProc, reinterpret_cast<LPARAM>(&data));

    Napi::Array result = Napi::Array::New(env);

    for (size_t i = 0; i < windowInfos.size(); ++i)
    {
        result[i] = windowInfos[i].ToObject(env);
    }
    return result;
}
#elif __APPLE__
Napi::Value GetAllWindowsInfo(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    Napi::Array result = Napi::Array::New(env);

    // macOS 实现：获取所有窗口信息
    CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionAll, kCGNullWindowID);
    if (!windowList)
        return result;

    CFIndex count = CFArrayGetCount(windowList);
    size_t windowIndex = 0;

    for (CFIndex i = 0; i < count; i++)
    {
        CFDictionaryRef windowInfo = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windowList, i));
        
        // 获取窗口所有者
        CFStringRef ownerName = static_cast<CFStringRef>(CFDictionaryGetValue(windowInfo, kCGWindowOwnerName));
        if (!ownerName)
            continue;

        char ownerNameC[256];
        CFStringGetCString(ownerName, ownerNameC, sizeof(ownerNameC));
        std::string ownerNameStr(ownerNameC);
        
        // 跳过系统进程
        std::vector<std::string> systemProcesses = {
            "SystemUIServer",
            "WindowServer",
            "Dock",
            "Finder"
        };
        
        bool isSystemProcess = false;
        for (const auto &process : systemProcesses)
        {
            if (ownerNameStr == process)
            {
                isSystemProcess = true;
                break;
            }
        }
        
        if (isSystemProcess)
            continue;

        // 获取窗口标题
        CFStringRef windowName = static_cast<CFStringRef>(CFDictionaryGetValue(windowInfo, kCGWindowName));
        if (!windowName)
            continue;

        char windowNameC[512];
        CFStringGetCString(windowName, windowNameC, sizeof(windowNameC));
        std::string windowTitle(windowNameC);
        
        if (windowTitle.empty())
            continue;

        // 获取窗口ID
        CFNumberRef windowIdRef = static_cast<CFNumberRef>(CFDictionaryGetValue(windowInfo, kCGWindowNumber));
        if (!windowIdRef)
            continue;

        CGWindowID windowId;
        CFNumberGetValue(windowIdRef, kCFNumberSInt32Type, &windowId);

        // 获取窗口位置和大小
        CFDictionaryRef boundsDict = static_cast<CFDictionaryRef>(CFDictionaryGetValue(windowInfo, kCGWindowBounds));
        if (!boundsDict)
            continue;

        CGRect bounds;
        CGRectMakeWithDictionaryRepresentation(boundsDict, &bounds);

        // 创建窗口信息对象
        Napi::Object windowObj = Napi::Object::New(env);
        windowObj.Set("windowId", Napi::Number::New(env, windowId));
        windowObj.Set("title", Napi::String::New(env, windowTitle));
        windowObj.Set("owner", Napi::String::New(env, ownerNameStr));
        windowObj.Set("x", Napi::Number::New(env, bounds.origin.x));
        windowObj.Set("y", Napi::Number::New(env, bounds.origin.y));
        windowObj.Set("width", Napi::Number::New(env, bounds.size.width));
        windowObj.Set("height", Napi::Number::New(env, bounds.size.height));

        result[windowIndex++] = windowObj;
    }

    CFRelease(windowList);
    return result;
}
#endif

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports.Set("switchWindowTopmostByTitle", Napi::Function::New(env, SwitchWindowTopmostByTitle));
    exports.Set("switchWindowTopmostByHWND", Napi::Function::New(env, SwitchWindowTopmostByHWND));
    exports.Set("getAllWindowsInfo", Napi::Function::New(env, GetAllWindowsInfo));
    exports.Set("getForegroundWindowAtPosition", Napi::Function::New(env, GetForegroundWindowAtPosition));
    return exports;
}

NODE_API_MODULE(window_nail, Init);