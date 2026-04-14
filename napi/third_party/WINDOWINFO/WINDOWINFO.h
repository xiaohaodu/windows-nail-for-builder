#include <napi.h>

#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#endif

class WindowInfo
{
public:
#ifdef _WIN32
  WindowInfo(Napi::Env env, HWND hwnd)
  {
    std::wstring titleW;
    int length = GetWindowTextLength(hwnd);
    if (length > 0)
    {
      titleW.resize(length + 1);
      GetWindowTextW(hwnd, &titleW[0], static_cast<int>(titleW.size()));
    }

    RECT rect;
    if (!GetWindowRect(hwnd, &rect))
    {
      throw Napi::Error::New(env, "Failed to get window rectangle");
    }

    title_ = WideStringToUtf8(titleW);
    left_ = rect.left;
    top_ = rect.top;
    right_ = rect.right;
    bottom_ = rect.bottom;
    hwnd_ = hwnd;
  }

  static std::string WideStringToUtf8(const std::wstring &wideStr)
  {
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), static_cast<int>(wideStr.size()), nullptr, 0, nullptr, nullptr);
    if (utf8Len <= 0)
    {
      throw std::runtime_error("Failed to convert wide string to UTF-8");
    }

    std::string utf8Str(utf8Len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), static_cast<int>(wideStr.size()), &utf8Str[0], utf8Len, nullptr, nullptr);

    return utf8Str;
  }

  Napi::Object ToObject(Napi::Env env) const
  {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("title", Napi::String::New(env, title_.c_str()));
    obj.Set("address", Napi::Number::New(env, reinterpret_cast<int64_t>(hwnd_)));
    obj.Set("left", Napi::Number::New(env, left_));
    obj.Set("top", Napi::Number::New(env, top_));
    obj.Set("right", Napi::Number::New(env, right_));
    obj.Set("bottom", Napi::Number::New(env, bottom_));
    return obj;
  }

  HWND hwnd_;
#elif __APPLE__
  WindowInfo(Napi::Env env, CGWindowID windowId)
  {
    // 获取窗口信息
    CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionAll, kCGNullWindowID);
    if (!windowList)
    {
      throw Napi::Error::New(env, "Failed to get window list");
    }

    bool found = false;
    CFIndex count = CFArrayGetCount(windowList);
    for (CFIndex i = 0; i < count; i++)
    {
      CFDictionaryRef windowInfo = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windowList, i));
      
      // 获取窗口ID
      CFNumberRef windowIdRef = static_cast<CFNumberRef>(CFDictionaryGetValue(windowInfo, kCGWindowNumber));
      if (!windowIdRef)
        continue;
        
      CGWindowID currentWindowId;
      CFNumberGetValue(windowIdRef, kCFNumberSInt32Type, &currentWindowId);
      
      if (currentWindowId != windowId)
        continue;
        
      // 获取窗口标题
      CFStringRef windowName = static_cast<CFStringRef>(CFDictionaryGetValue(windowInfo, kCGWindowName));
      if (windowName)
      {
        char titleBuffer[512];
        CFStringGetCString(windowName, titleBuffer, sizeof(titleBuffer));
        title_ = titleBuffer;
      }
      
      // 获取窗口位置和大小
      CFDictionaryRef boundsDict = static_cast<CFDictionaryRef>(CFDictionaryGetValue(windowInfo, kCGWindowBounds));
      if (boundsDict)
      {
        CGRect bounds;
        CGRectMakeWithDictionaryRepresentation(boundsDict, &bounds);
        left_ = static_cast<int>(bounds.origin.x);
        top_ = static_cast<int>(bounds.origin.y);
        right_ = static_cast<int>(bounds.origin.x + bounds.size.width);
        bottom_ = static_cast<int>(bounds.origin.y + bounds.size.height);
      }
      
      windowId_ = windowId;
      found = true;
      break;
    }

    CFRelease(windowList);
    
    if (!found)
    {
      throw Napi::Error::New(env, "Failed to find window");
    }
  }

  Napi::Object ToObject(Napi::Env env) const
  {
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("title", Napi::String::New(env, title_.c_str()));
    obj.Set("address", Napi::Number::New(env, static_cast<int64_t>(windowId_)));
    obj.Set("left", Napi::Number::New(env, left_));
    obj.Set("top", Napi::Number::New(env, top_));
    obj.Set("right", Napi::Number::New(env, right_));
    obj.Set("bottom", Napi::Number::New(env, bottom_));
    return obj;
  }

  CGWindowID windowId_;
#endif

  std::string title_;
  int left_, top_, right_, bottom_;
};
