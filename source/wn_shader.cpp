//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-26 16:49:45
//

#include <atlbase.h>
#include <dxcapi.h>
#include <wrl/client.h>
#include <sstream>

#include "wn_shader.h"
#include "wn_filesystem.h"
#include "wn_output.h"
#include "wn_util.h"

const char *GetProfileFromType(shader_type type)
{
    switch (type) {
        case ShaderType_Vertex: {
            return "vs_6_0";
        }
        case ShaderType_Pixel: {
            return "ps_6_0";
        }
        case ShaderType_Compute: {
            return "cs_6_0";
        }
    }
    return "???";
}

compiled_shader shader_compile(const std::string& path, shader_type type)
{
    compiled_shader shader = {};
    bool compile = false;

    if (!fs_exists(".cache")) {
        fs_createdir(".cache/");
    }

    /// @note(ame): Check if the thingy is not already in cache
    std::stringstream ss;
    ss << ".cache/" << wn_hash(path.c_str(), path.size(), 1000) << ".wns";
    if (fs_exists(ss.str())) {
        /// @note(ame): exists, load the bitch in and thats it
        shader_header header;

        FILE* f = fopen(ss.str().c_str(), "rb");
        fread(&header, sizeof(shader_header), 1, f);
        
        /// @note(ame): compare filetimes
        u32 uncached_low, uncached_high;
        fs_getfiletime(path, uncached_low, uncached_high);
        if (uncached_low == header.low_time && uncached_high == header.high_time) {
            shader.bytes.resize(header.size);
            fread(shader.bytes.data(), header.size, 1, f);
            log("[shader] Loaded cached shader %s", path.c_str());
            return shader;
        } else {
            log("[shader] Shader %s was modified -- recompiling", path.c_str());
            compile = true;
        }
    } else {
        compile = true;
    }

    if (compile) {
        using namespace Microsoft::WRL;

        std::string source = fs_readtext(path);

        wchar_t wide_target[512];
        swprintf_s(wide_target, 512, L"%hs", GetProfileFromType(type));

        wchar_t* wide_entry = L"Main";

        ComPtr<IDxcUtils> utils;
        ComPtr<IDxcCompiler> compiler;
        if (!SUCCEEDED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)))) {
            throw_error("DXC: Failed to create DXC utils instance!");
        }
        if (!SUCCEEDED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)))) {
            throw_error("DXC: Failed to create DXC compiler instance!");
        }

        ComPtr<IDxcIncludeHandler> include_handler;
        if (!SUCCEEDED(utils->CreateDefaultIncludeHandler(&include_handler))) {
            throw_error("DXC: Failed to create default include handler!");
        }

        ComPtr<IDxcBlobEncoding> source_blob;
        if (!SUCCEEDED(utils->CreateBlob(source.c_str(), source.size(), 0, &source_blob))) {
            throw_error("DXC: Failed to create output blob!");
        }

        LPCWSTR args[] = {
            L"-Zi",
            L"-Fd",
            L"-Fre",
            L"-Qembed_debug", /// @todo(ame): remove
            L"-Wno-payload-access-perf",
            L"-Wno-payload-access-shader"
        };

        ComPtr<IDxcOperationResult> result;
        if (!SUCCEEDED(compiler->Compile(source_blob.Get(), L"Shader", wide_entry, wide_target, args, ARRAYSIZE(args), nullptr, 0, include_handler.Get(), &result))) {
            log("[DXC] DXC: Failed to compile shader!");
        }

        ComPtr<IDxcBlobEncoding> errors;
        result->GetErrorBuffer(&errors);

        if (errors && errors->GetBufferSize() != 0) {
            ComPtr<IDxcBlobUtf8> error_blob;
            errors->QueryInterface(IID_PPV_ARGS(&error_blob));
            log("DXC Error: %s", error_blob->GetStringPointer());
            shader.errors = true;
        }

        ComPtr<IDxcBlob> shader_blob;
        result->GetResult(&shader_blob);

        shader.bytes.resize(shader_blob->GetBufferSize());
        memcpy(shader.bytes.data(), shader_blob->GetBufferPointer(), shader.bytes.size());
    
        u32 uncached_low, uncached_high;
        fs_getfiletime(path, uncached_low, uncached_high);

        shader_header header;
        header.high_time = uncached_high;
        header.low_time = uncached_low;
        header.type = type;
        header.size = shader.bytes.size();

        FILE* f = fopen(ss.str().c_str(), "wb+");
        fwrite(&header, sizeof(header), 1, f);
        fwrite(shader.bytes.data(), shader.bytes.size(), 1, f);
        fclose(f);

        log("[shader] Compiled and cached shader %s", path.c_str());
    }
    return shader;
}
