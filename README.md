# Windows Nail Builder - Windows 窗口置顶工具（优化版）

一个简单易用的 Windows 窗口置顶工具，帮助您快速将任意窗口置顶显示。

这是 **windows-nail** 的优化版本，使用 electron-builder 和 Vite 构建。

## ✨ 特性

- 🖥️ **窗口置顶** - 快速将任意窗口设置为置顶状态
- 🎯 **精准选择** - 支持通过窗口标题或鼠标位置选择窗口
- 📋 **窗口列表** - 显示所有可操作的窗口列表
- 🚀 **原生性能** - 使用 C++ Node.js 扩展实现高性能操作
- 💡 **Vue 3 + TypeScript** - 现代化的前端技术栈
- 📦 **Electron Builder** - 完善的安装包引导和构建流程
- 🔄 **自动更新** - 集成 electron-updater 支持自动更新

## 🛠️ 技术栈

### 前端
- **Vue 3** - 渐进式 JavaScript 框架
- **TypeScript** - 类型安全的 JavaScript 超集
- **Vite** - 下一代前端构建工具
- **Sass** - CSS 预处理器

### 原生模块
- **C++ Node.js 扩展** - 核心窗口操作功能
- **Node-API (N-API)** - Node.js 原生模块接口

### 桌面应用
- **Electron** - 跨平台桌面应用框架
- **Electron Builder** - 应用构建和打包工具
- **Electron Updater** - 自动更新功能

## 📦 安装

### 环境要求

- Windows 操作系统
- Node.js (建议 v18 或更高版本)
- Python 3.x (用于 node-gyp 编译)
- Visual Studio Build Tools (用于 C++ 编译)

### 克隆项目

```bash
git clone https://github.com/xiaohaodu/windows-nail-for-builder.git
cd windows-nail-for-builder
```

### 安装依赖

```bash
npm install
```

## 🚀 使用

### 开发模式

```bash
npm run dev
```

### 构建应用

```bash
npm run build
```

### 发布应用

```bash
npm run publish
```

### 预览

```bash
npm run preview
```

## 📖 API 功能

### 核心功能

- `switchWindowTopmostByTitle(title, isTopmost)` - 通过窗口标题设置置顶
- `switchWindowTopmostByHWND(hwnd, isTopmost)` - 通过窗口句柄设置置顶
- `getAllWindowsInfo()` - 获取所有窗口信息
- `getForegroundWindowAtPosition(x, y)` - 获取指定位置的窗口

## 📁 项目结构

```
windows-nail-for-builder/
├── napi/                    # C++ 扩展源代码
│   ├── window_nail.cc      # 主扩展实现
│   ├── index.js            # Node.js 入口
│   ├── CMakeLists.txt      # CMake 构建配置
│   ├── test/               # 测试代码
│   └── third_party/        # 第三方库
│       ├── LOG/            # 日志模块
│       └── WINDOWINFO/     # 窗口信息模块
├── electron/                # Electron 主进程代码
│   ├── main.ts             # 主进程入口
│   ├── preload.ts          # 预加载脚本
│   ├── Tary.ts             # 托盘相关
│   └── Window.ts           # 窗口管理
├── src/                     # Vue 前端源代码
├── package.json             # 项目配置
├── electron-builder.json5   # Electron Builder 配置
├── vite.config.ts          # Vite 配置
├── binding.gyp             # node-gyp 构建配置
└── README.md               # 项目文档
```

## 🔄 与 windows-nail 的区别

| 特性 | windows-nail | windows-nail-for-builder |
|------|-------------|------------------------|
| 构建工具 | Electron Forge | Electron Builder |
| 前端构建 | - | Vite |
| 安装包引导 | 无 | 有 |
| 代码模板 | 较少 | 较多 |
| Vue 3 | ❌ | ✅ |
| TypeScript | ❌ | ✅ |

## 🤝 贡献

欢迎提出 Issue 和 Pull Request！

如有新需求，可联系微信：18713908616（请注明来意）

## 📄 许可证

ISC License

## 👤 作者

duxiaohao
