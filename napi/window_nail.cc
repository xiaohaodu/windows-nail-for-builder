#include <napi.h>
#include <windows.h>
#include <iostream>
#include <tchar.h>
#include <vector>
#include <string>

#include "third_party/WINDOWINFO/WINDOWINFO.h"
#include "third_party/LOG/LOG.h"

bool IsSystemWindowClass(HWND hwnd)
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

struct EnumData
{
    POINT point;
    HWND hwndResult;
    EnumData(int x, int y) : point{x, y}, hwndResult{nullptr} {}
};

Napi::Value SwitchWindowTopmostByTitle(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsBoolean())
    {
        Napi::TypeError::New(env, "First argument must be a window title string, and second argument must be a boolean indicating topmost status").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    std::u16string windowTitleU16 = info[0].As<Napi::String>().Utf16Value();
    std::wstring windowTitle(windowTitleU16.begin(), windowTitleU16.end());

    HWND hWndTarget = FindWindowW(NULL, windowTitle.c_str());
    if (hWndTarget == NULL)
    {
        Napi::Error::New(env, "Failed to find the target window").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    bool setToTopmost = info[1].As<Napi::Boolean>().Value();
    HWND insertAfter = setToTopmost ? HWND_TOPMOST : HWND_NOTOPMOST;

    // 设置窗口层级
    if (!SetWindowPos(hWndTarget, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE))
    {
        std::string errorMsg = setToTopmost ? "Failed to set the window as topmost" : "Failed to set the window as normal";
        Napi::Error::New(env, errorMsg).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

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
    uint32_t hWndTargetValue = info[0].As<Napi::Number>().Uint32Value();
    HWND hWndTarget = reinterpret_cast<HWND>(static_cast<uintptr_t>(hWndTargetValue));

    // 检查窗口句柄有效性
    if (!IsWindow(hWndTarget)) // 使用Windows API IsWindow检查窗口句柄是否有效
    {
        // 窗口句柄无效，抛出异常并返回false
        Napi::Error::New(env, "Invalid window handle provided").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    // 获取设置置顶状态的布尔值
    bool setToTopmost = info[1].As<Napi::Boolean>().Value();
    HWND insertAfter = setToTopmost ? HWND_TOPMOST : HWND_NOTOPMOST;

    // 设置窗口层级
    if (!SetWindowPos(hWndTarget, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE))
    {
        std::string errorMsg = setToTopmost ? "Failed to set the window as topmost" : "Failed to set the window as normal";
        // 设置窗口层级失败，抛出异常并返回false
        Napi::Error::New(env, errorMsg).ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    // 记录操作结果
    std::string logMsg = setToTopmost ? "Successfully set window to topmost" : "Successfully set window to normal";
    Log::LogMessage(env, logMsg);

    // 成功执行，返回true
    return Napi::Boolean::New(env, true);
}

std::wstring GetWindowTitle(HWND hWnd)
{
    wchar_t titleBuffer[512];
    int length = GetWindowTextW(hWnd, titleBuffer, ARRAYSIZE(titleBuffer));
    return std::wstring(titleBuffer, length);
}

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

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports.Set("switchWindowTopmostByTitle", Napi::Function::New(env, SwitchWindowTopmostByTitle));
    exports.Set("switchWindowTopmostByHWND", Napi::Function::New(env, SwitchWindowTopmostByHWND));
    exports.Set("getAllWindowsInfo", Napi::Function::New(env, GetAllWindowsInfo));
    exports.Set("getForegroundWindowAtPosition", Napi::Function::New(env, GetForegroundWindowAtPosition));
    return exports;
}

NODE_API_MODULE(window_nail, Init);