//
// $Notice: Xander Studios @ 2024
// $Author: Amélie Heinrich
// $Create Time: 2024-10-31 20:55:12
//

#include <cassert>
#include <iostream>
#include <string>

#include "angel/scriptbuilder/scriptbuilder.h"
#include "angel/scriptstdstring/scriptstdstring.h"
#include "angel/scriptarray/scriptarray.h"
#include "angel/scriptdictionary/scriptdictionary.h"
#include "angel/scriptfile/scriptfile.h"
#include "angel/scriptfile/scriptfilesystem.h"
#include "angel/scripthelper/scripthelper.h"
#include "angel/scripthandle/scripthandle.h"
#include "angel/debugger/debugger.h"
#include "angel/contextmgr/contextmgr.h"
#include "angel/datetime/datetime.h"

#include "wn_script.h"
#include "wn_output.h"
#include "wn_filesystem.h"

/// @note(ame): Script function definitions
void script_engine_configure();
void script_execute(game_script *s, asIScriptFunction *function, void** out = nullptr);
///

script_system script;

void message_callback(const asSMessageInfo *msg, void *param)
{
	const char *type = "ERR ";
	if( msg->type == asMSGTYPE_WARNING ) 
		type = "WARN";
	else if( msg->type == asMSGTYPE_INFORMATION ) 
		type = "INFO";

	log("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
}

void script_system_init()
{
    script.engine = asCreateScriptEngine();
    if (script.engine == nullptr) {
        log("[angelscript] failed to create script engine!");
        throw_error("Angel Script Error");
    }
    
    script.engine->SetMessageCallback(asFUNCTION(message_callback), 0, asCALL_CDECL);

    script_engine_configure();

    log("[angelscript] initialized script engine");
}

game_script* script_system_load_or_get_script(const std::string& path, const std::string& class_name)
{
    if (script.script_cache.count(path) > 0) {
        return &script.script_cache[path];
    } else {
        script.script_cache[path] = {};
        game_script_load(&script.script_cache[path], path.c_str(), class_name.c_str());
    }
}

void script_system_exit()
{
    script.engine->ShutDownAndRelease();
}

void game_script_load(game_script *s, const char* path, const char* script_name)
{
    /// @note(ame): load and compile script
    auto contents = fs_readtext(path);
    s->module = script.engine->GetModule(path, asGM_ALWAYS_CREATE);
    if (!s->module) {
        log("[angelscript] Failed to create module!");
        throw_error("Angel Script Error");
    }

    i32 r = s->module->AddScriptSection("script", &contents[0], contents.size());
    if (r < 0) {
        log("[angelscript] Failed to add script section");
        throw_error("Angel Script Error");
    }

    r = s->module->Build();
    if (r < 0) {
        log("[angelscript] Failed to compile script!");
        throw_error("Angel Script Error");
    } else {
        log("[angelscript] Compiled script %s", path);
    }

    /// @note(ame): create context and get functions
    s->ctx = script.engine->CreateContext();
    if (s->ctx == nullptr) {
        log("[angelscript] Failed to create script context");
        throw_error("Angel Script Error");
    }

    asITypeInfo *type = nullptr;
    i32 tc = s->module->GetObjectTypeCount();
    for (i32 n = 0; n < tc; n++) {
        bool found = false;
        type = s->module->GetObjectTypeByIndex(n);
        if (!strcmp(type->GetName(), script_name)) {
            found = true;
        }

        if (found == true) {
            s->type = type;
            break;
        }
    }

    if (s->type == nullptr) {
        log("[angelscript] Script must have Behaviour interface");
        throw_error("Angel Script Error!");
    }

    std::string str = std::string(type->GetName()) + "@ " + std::string(type->GetName()) + "()";
    s->start = s->type->GetFactoryByDecl(str.c_str());
    if (s->start == nullptr) {
        log("[angelscript] Failed to find start function!");
    }

    s->update = s->type->GetMethodByDecl("void Update()");
    if (s->update == nullptr) {
        log("[angelscript] Failed to find update function!");
    }

    script_execute(s, s->start, reinterpret_cast<void**>(&s->instance));
}

void game_script_execute(game_script *s)
{
    script_execute(s, s->update);
}

void game_script_free(game_script *s)
{
    s->instance->Release();
    s->ctx->Release();
}

/// @note(ame): Function implementations for scripts and utils

void PrintString(std::string &str)
{
	std::cout << str;
}

void script_execute(game_script *s, asIScriptFunction *function, void** out)
{
    i32 r = s->ctx->Prepare(function);
    if (r < 0) {
        log("[angelscript] failed to prepare context");
        throw_error("Angel Script Error");
    }

    if (s->instance) {
        s->ctx->SetObject(s->instance);
    }
    
    r = s->ctx->Execute();
    if (r != asEXECUTION_FINISHED) {
		if (r == asEXECUTION_EXCEPTION)
		{
			std::cout << "Exception: " << s->ctx->GetExceptionString() << std::endl;
			std::cout << "Function: " << s->ctx->GetExceptionFunction()->GetDeclaration() << std::endl;
			std::cout << "Line: " << s->ctx->GetExceptionLineNumber() << std::endl;

			// It is possible to print more information about the location of the 
			// exception, for example the call stack, values of variables, etc if 
			// that is of interest.
		}
	} else {
        if (out != nullptr) {
            asIScriptObject *obj = 0;
            obj = *((asIScriptObject**)s->ctx->GetAddressOfReturnValue());
            obj->AddRef();
            *out = obj;
        }
    }

    s->ctx->Unprepare();
}

void script_engine_configure()
{
    RegisterStdString(script.engine);
    RegisterScriptArray(script.engine, false);
    RegisterStdStringUtils(script.engine);
    RegisterScriptDictionary(script.engine);
    RegisterScriptDateTime(script.engine);
    RegisterScriptFile(script.engine);
    RegisterScriptFileSystem(script.engine);
    RegisterScriptHandle(script.engine);
    RegisterExceptionRoutines(script.engine);

    u32 r = script.engine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(PrintString), asCALL_CDECL); assert(r >= 0);
}
