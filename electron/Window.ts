import { app, BrowserWindow } from "electron";
import { autoUpdater } from "electron-updater";
import path from "path";
const top_windows = require("bindings")("window_nail");

interface WindowInfo {
  address: number | string;
  top?: boolean;
  title: string;
  // Add any additional properties from the original `WindowInfo` type as needed
}

// The built directory structure
//
// ├─┬─┬ dist
// │ │ └── index.html
// │ │
// │ ├─┬ dist-electron
// │ │ ├── main.js
// │ │ └── preload.js
// │
process.env.APP_ROOT = path.join(__dirname, "..");
// 🚧 Use ['ENV_NAME'] avoid vite:define plugin - Vite@2.x
export const VITE_DEV_SERVER_URL = process.env["VITE_DEV_SERVER_URL"];
export const MAIN_DIST = path.join(process.env.APP_ROOT, "dist-electron");
export const RENDERER_DIST = path.join(process.env.APP_ROOT, "dist");

process.env.VITE_PUBLIC = VITE_DEV_SERVER_URL
  ? path.join(process.env.APP_ROOT, "public")
  : RENDERER_DIST;

class Window {
  win: BrowserWindow | null = null;
  private topWindow: WindowInfo | null = null;
  private windows: WindowInfo[] = [];
  private moveTimeout: NodeJS.Timeout | null = null;

  constructor() {
    app.whenReady().then(() => {
      this.createWindow();
    });

    this.getAllWindowsInfo();
  }

  createWindow() {
    this.win = new BrowserWindow({
      height: 50,
      width: 50,
      transparent: true,
      alwaysOnTop: true,
      skipTaskbar: false,
      focusable: false,
      titleBarStyle: "hidden",
      frame: false,
      webPreferences: {
        preload: path.join(__dirname, "preload.js"),
        devTools: true,
        backgroundThrottling: false,
      },
      title: "",
      resizable: false,
      useContentSize: false,
      hasShadow: false,
      visualEffectState: "active",
      backgroundMaterial: "none",
      titleBarOverlay: false,
    });

    if (process.platform === "win32") {
      this.win.setAlwaysOnTop(true, "screen-saver");
    }

    if (VITE_DEV_SERVER_URL) {
      this.win.loadURL(VITE_DEV_SERVER_URL);
    } else {
      this.win.loadFile(path.join(RENDERER_DIST, "index.html"));
    }

    this.win.on("closed", () => {
      this.close();
      this.moveTimeout && clearTimeout(this.moveTimeout);
    });
  }

  getAllWindowsInfo() {
    const windows = top_windows.getAllWindowsInfo();
    const windowsTemp = [...this.windows]; // 创建深拷贝避免引用问题
    this.windows = [];

    if (windowsTemp && windowsTemp.length > 0) {
      for (const i in windows) {
        let flag = false;
        for (const j in windowsTemp) {
          if (windowsTemp[j].address === windows[i].address) {
            flag = true;
            this.windows.push({ ...windowsTemp[j] });
            break;
          }
        }
        if (!flag) this.windows.push({ ...windows[i], top: false });
      }
    } else {
      this.windows = windows.map((window: any) => ({ ...window, top: false }));
    }

    return this.windows;
  }

  switchWindowTopmostByHWND(hwnd: number | string) {
    const hwndNum = typeof hwnd === 'string' ? parseInt(hwnd) : hwnd;
    
    for (const i in this.windows) {
      const windowAddress = typeof this.windows[i].address === 'string' 
        ? parseInt(this.windows[i].address) 
        : this.windows[i].address;
        
      if (windowAddress === hwndNum) {
        if (!this.windows[i].top) {
          this.windows[i].top = true;
          top_windows.switchWindowTopmostByHWND(this.windows[i].address, true);
        }
      } else {
        if (this.windows[i].top) {
          this.windows[i].top = false;
          top_windows.switchWindowTopmostByHWND(this.windows[i].address, false);
        }
      }
    }
  }

  setWindowsNail(x: number, y: number) {
    const res = top_windows.getForegroundWindowAtPosition(x, y) as WindowInfo;
    if (this.topWindow) {
      if (res) {
        // 比较地址时确保类型一致
        const topWindowAddr = typeof this.topWindow.address === 'string' 
          ? parseInt(this.topWindow.address) 
          : this.topWindow.address;
        const resAddr = typeof res.address === 'string' 
          ? parseInt(res.address) 
          : res.address;
          
        if (resAddr !== topWindowAddr) {
          top_windows.switchWindowTopmostByHWND(this.topWindow.address, false);
          this.topWindow = res;
          top_windows.switchWindowTopmostByHWND(this.topWindow.address, true);
        }
      } else {
        top_windows.switchWindowTopmostByHWND(this.topWindow.address, false);
        this.topWindow = null;
      }
    } else {
      if (res) {
        this.topWindow = res;
        top_windows.switchWindowTopmostByHWND(this.topWindow.address, true);
      }
    }
  }

  close() {
    if (this.topWindow) {
      top_windows.switchWindowTopmostByHWND(this.topWindow.address, false);
      this.topWindow = null;
    } else {
      for (const i in this.windows) {
        if (this.windows[i].top) {
          top_windows.switchWindowTopmostByHWND(this.windows[i].address, false);
        }
      }
    }
  }

  openDevTool() {
    app.whenReady().then(() => {
      this.win?.webContents.openDevTools();
    });
  }

  autoUpdateApp() {
    autoUpdater.checkForUpdatesAndNotify();
  }

  // This function was empty in the original code, so it's left unchanged.
  importDeveloperTool() {}
}

export default Window;
