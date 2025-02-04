#include "GarrysMod/Lua/Interface.h"
#include <GarrysMod/Lua/Types.h>
#include <Platform.hpp>
#include <GarrysMod/ModuleLoader.hpp>
#include "VTable.h"

#ifdef _WIN32
#define CREATELUAINTERFACE 4
#define CLOSELUAINTERFACE 5
#define RUNSTRINGEX 111
#else
#define CREATELUAINTERFACE 5
#define CLOSELUAINTERFACE 6
#define RUNSTRINGEX 111
#endif

#ifndef _WIN32
#define __cdecl
#define __thiscall
#define __fastcall
#endif

typedef unsigned char uchar; 

VTable* sharedHooker;
VTable* clientHooker;

using namespace GarrysMod;

typedef void* (__cdecl* CreateInterfaceFn)(const char* name, int* found);
const SourceSDK::ModuleLoader lua_shared_loader("lua_shared");

Lua::ILuaBase* MENU;
lua_State* clientState;

typedef void* (__thiscall* hRunStringExFn)(void*, char const*, char const*, char const*, bool, bool, bool, bool);
#if ARCHITECTURE_IS_X86_64
void* __fastcall hRunStringEx(void* _this, const char* fileName, const char* path, const char* str, bool bRun, bool bPrintErrors, bool bDontPushErrors, bool bNoReturns)
#else
void* __fastcall hRunStringEx(void* _this, void* unknown, const char* fileName, const char* path, const char* str, bool bRun, bool bPrintErrors, bool bDontPushErrors, bool bNoReturns)
#endif
{
	MENU->PushSpecial(Lua::SPECIAL_GLOB);
	MENU->GetField(-1, "hook");
	MENU->GetField(-1, "Run");
	MENU->PushString("RunOnClient");
	MENU->PushString(fileName);
	MENU->PushString(str);
	MENU->Call(3, 1);

	int type = MENU->GetType(-1);
	switch (type) {
		case (int)GarrysMod::Lua::Type::String:
		{
			str = MENU->GetString(-1);
			MENU->Pop(1);
		}
			break;
		case (int)GarrysMod::Lua::Type::Bool:
		{
			bool ret = MENU->GetBool(-1);
			MENU->Pop(1);

			if (ret == false) {
				MENU->Pop(2);
				return 0;
			}
		}
			break;
		default:
			MENU->Pop(1);
	}

	MENU->Pop(2);

	return hRunStringExFn(clientHooker->getold(RUNSTRINGEX))(_this, fileName, path, str, bRun, bPrintErrors, bDontPushErrors, bNoReturns);
}

typedef void* (__thiscall* hCreateLuaInterfaceFn)(void*, uchar, bool);
#if ARCHITECTURE_IS_X86_64
void* hCreateLuaInterface(void* _this, uchar stateType, bool renew)
#else
void* __fastcall hCreateLuaInterface(void* _this, void* unknown, uchar stateType, bool renew)
#endif
{
	lua_State* state = reinterpret_cast<lua_State*>(hCreateLuaInterfaceFn(sharedHooker->getold(CREATELUAINTERFACE))(_this, stateType, renew));

	MENU->PushSpecial(Lua::SPECIAL_GLOB);
	MENU->GetField(-1, "hook");
		MENU->GetField(-1, "Run");
			MENU->PushString("ClientStateCreated");
		MENU->Call(1, 0);
	MENU->Pop(2);

	if (stateType != 0)
		return state;

	clientState = state;
	clientHooker = new VTable(clientState);
	clientHooker->hook(RUNSTRINGEX, (void*)&hRunStringEx);

	return clientState;
}

typedef void* (__thiscall* hCloseLuaInterfaceFn)(void*, void*);
#if ARCHITECTURE_IS_X86_64
void* hCloseLuaInterface(void* _this, lua_State* luaInterface)
#else
void* __fastcall hCloseLuaInterface(void* _this, void* unknown, void* luaInterface)
#endif
{
	MENU->PushSpecial(Lua::SPECIAL_GLOB);
	MENU->GetField(-1, "hook");
		MENU->GetField(-1, "Run");
			MENU->PushString("ClientStateDestroyed");
		MENU->Call(1, 0);
	MENU->Pop(2);

	if (luaInterface == clientState)
		clientState = nullptr;

	return hCloseLuaInterfaceFn(sharedHooker->getold(CLOSELUAINTERFACE))(_this, luaInterface);
}

class CLuaInterface
{
private:
	template<typename T>
	inline T get(unsigned short which)
	{
		return T((*(char ***)(this))[which]);
	}

public:
	void RunStringEx(const char* fileName, const char* path, const char* str, bool run = true, bool showErrors = true, bool pushErrors = true, bool noReturns = true)
	{
		return get<void(__thiscall*)(void*, char const*, char const*, char const*, bool, bool, bool, bool)>(RUNSTRINGEX)(this, fileName, path, str, run, showErrors, pushErrors, noReturns);
	}
};

LUA_FUNCTION(RunOnClient) {
	if (!clientState) 
	{
		LUA->ThrowError("Not in game");
		return 0;
	}

	try {
		const char* str = LUA->CheckString(-1);
		reinterpret_cast<CLuaInterface*>(clientState)->RunStringEx("", "", str);
	}
	catch (const char* err) 
	{
		LUA->ThrowError(err);
	}

	return 0;
}

GMOD_MODULE_OPEN()
{
	MENU = LUA;

	void* luaShared = CreateInterfaceFn(lua_shared_loader.GetSymbol("CreateInterface"))("LUASHARED003", 0);
	if (!luaShared)
	{
		MENU->ThrowError("Can't get lua shared interface");
		return 0;
	}

	sharedHooker = new VTable(luaShared);
	sharedHooker->hook(CREATELUAINTERFACE, (void*)&hCreateLuaInterface);
	sharedHooker->hook(CLOSELUAINTERFACE, (void*)&hCloseLuaInterface);
	
	MENU->PushSpecial(Lua::SPECIAL_GLOB);
	MENU->PushString("RunOnClient");
	MENU->PushCFunction(RunOnClient);
	MENU->SetTable(-3);
	MENU->Pop();

	return 0;
}