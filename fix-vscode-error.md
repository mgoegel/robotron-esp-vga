Damit VS Code nicht an den includes meckert, muss folgendes in die .vscode\c_cpp_properties.json:

Bei include:

    "${config:idf.espIdfPath}/components/**"

Und in der configuration:

    "compileCommands": "${workspaceFolder}/build/
    
```
{
    "configurations": [
        {
            "name": "Win32",
            "includePath": [
                "${workspaceFolder}/**",
                "${config:idf.espIdfPath}/components/**"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE"
            ],
            "windowsSdkVersion": "10.0.19041.0",
            "compilerPath": "cl.exe",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "windows-msvc-x64",
            "compileCommands": "${workspaceFolder}/build/compile_commands.json"
        }
    ],
    "version": 4
}
```