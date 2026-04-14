{
    "targets": [
        {
            "target_name": "window_nail",
            "sources": ["napi/window_nail.cc", "napi/third_party/LOG/LOG.cpp"],
            "include_dirs": ["<!(node -p \"require('node-addon-api').include_dir\")"],
            "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"],
            "defines": ["NAPI_CPP_EXCEPTIONS"],
            "cflags!": ["-fno-exceptions"],
            "cflags_cc!": ["-fno-exceptions"],
            "conditions": [
                ['OS=="win"', {"libraries": ["User32.lib"]}],
                ['OS=="mac"', {
                    "libraries": [
                        "-framework CoreGraphics",
                        "-framework Carbon",
                        "-framework ApplicationServices"
                    ]
                }]
            ],
            "msvs_settings": {
                "VCCLCompilerTool": {"AdditionalOptions": ["/utf-8", "/EHsc"]}
            },
        },
    ],
}
