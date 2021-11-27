#include "GarrysMod/Lua/Interface.h"
#include "VTable.h"
#include "Util.h"
#include "windows.h"

#define CREATELUAINTERFACE 4
#define CLOSELUAINTERFACE 5
#define RUNSTRINGEX 111

typedef unsigned char uchar;

VTable* sharedHooker;
VTable* clientHooker;

using namespace GarrysMod;

Lua::ILuaBase *MENU;
void *clientState;

typedef void *(__thiscall *hRunStringExFn)(void*, char const*, char const*, char const*, bool, bool, bool, bool);
void * __fastcall hRunStringEx(void *_this, const char* filename, const char* path, const char* str, bool bRun, bool bPrintErrors, bool bDontPushErrors, bool bNoReturns)
{
	/*MENU->PushSpecial(Lua::SPECIAL_GLOB);
	MENU->GetField(-1, "hook");
		MENU->GetField(-1, "Call");
			MENU->PushString("RunOnClient");
			MENU->PushNil();
			MENU->PushString(filename);
			MENU->PushString(stringToRun);
		MENU->Call(4, 1);

		if (!MENU->IsType(-1, Lua::Type::NIL))
			stringToRun = MENU->CheckString();
	MENU->Pop(3);*/

	return hRunStringExFn(clientHooker->getold(RUNSTRINGEX))(_this, filename, path, str, bRun, bPrintErrors, bDontPushErrors, bNoReturns);
}

typedef void* (__fastcall* CreateLuaInterfaceFn)(void*, uchar, bool);
void * __fastcall hCreateLuaInterface(void *_this, uchar stateType, bool renew)
{
	void *state = CreateLuaInterfaceFn(sharedHooker->getold(CREATELUAINTERFACE))(_this, stateType, renew);

	/*MENU->PushSpecial(Lua::SPECIAL_GLOB);
	MENU->GetField(-1, "hook");
		MENU->GetField(-1, "Call");
			MENU->PushString("ClientStateCreated");
			MENU->PushNil();
		MENU->Call(2, 0);
	MENU->Pop(2);*/

	if (stateType != 0)
		return state;

	clientState = state;

	clientHooker = new VTable(clientState);
	clientHooker->hook(RUNSTRINGEX, hRunStringEx);

	return clientState;
}

typedef void *(__thiscall *hCloseLuaInterfaceFn)(void*, void*);
void* __fastcall hCloseLuaInterface(void* _this, void* luaInterface)
{
	if (luaInterface == clientState)
		clientState = NULL;

	return hCloseLuaInterfaceFn(sharedHooker->getold(CLOSELUAINTERFACE))(_this, luaInterface);
}

class CLuaInterface
{
private:
	template<typename t>
	inline t get(unsigned short which)
	{
		return t((*(char ***)(this))[which]);
	}

public:
	void RunStringEx(const char* filename, const char* path, const char* str, bool bRun = true, bool bPrintErrors = true, bool bDontPushErrors = true, bool bNoReturns = true)
	{
		return get<void(__thiscall *)(void*, char const*, char const*, char const*, bool, bool, bool, bool)>(RUNSTRINGEX)(this, filename, path, str, bRun, bPrintErrors, bDontPushErrors, bNoReturns); //free cookies for people that know how to detect stuff
	}

};

LUA_FUNCTION(RunOnClient) {
	if (!clientState)
		LUA->ThrowError("Not in game");

	try {
		const char* filename = LUA->CheckString(-3);
		const char* path = LUA->CheckString(-2);
		const char* str = LUA->CheckString(-1);

		reinterpret_cast<CLuaInterface*>(clientState)->RunStringEx(filename, path, str);
	}
	catch (const char* err) {
		LUA->ThrowError(err);
	}

	return 0;
}

GMOD_MODULE_OPEN()
{
	MENU = LUA;

	auto luaShared = util::GetInterfaceSingle<void *>("lua_shared.dll", "LUASHARED003");
	if (!luaShared)
		MessageBoxA(NULL, "gay", "gay", NULL);

	sharedHooker = new VTable(luaShared);

	sharedHooker->hook(CREATELUAINTERFACE, hCreateLuaInterface);
	sharedHooker->hook(CLOSELUAINTERFACE, hCloseLuaInterface);
	
	LUA->PushSpecial(Lua::SPECIAL_GLOB);
	LUA->PushString("RunOnClient");
	LUA->PushCFunction(RunOnClient);
	LUA->SetTable(-3);
	LUA->Pop();

	return 0;
}