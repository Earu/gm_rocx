#include "GarrysMod/Lua/Interface.h"
#include "GarrysMod/Lua/Types.h"
#include "VTable.h"
#include "Util.h"
#include "windows.h"

constexpr int CREATELUAINTERFACE = 4;
constexpr int CLOSELUAINTERFACE = 5;
constexpr int RUNSTRINGEX = 111;

typedef unsigned char uchar;

VTable* sharedHooker;
VTable* clientHooker;

using namespace GarrysMod;

Lua::ILuaBase *MENU;
void *clientState;

typedef void *(__thiscall *hRunStringExFn)(void*, char const*, char const*, char const*, bool, bool, bool, bool);
void * __fastcall hRunStringEx(void *_this, const char* fileName, const char* path, const char* str, bool bRun, bool bPrintErrors, bool bDontPushErrors, bool bNoReturns)
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
				return hRunStringExFn();
			}
		}
			break;
		default:
			MENU->Pop(1);
	}

	MENU->Pop(2);

	return hRunStringExFn(clientHooker->getold(RUNSTRINGEX))(_this, fileName, path, str, bRun, bPrintErrors, bDontPushErrors, bNoReturns);
}

typedef void* (__fastcall* CreateLuaInterfaceFn)(void*, uchar, bool);
void * __fastcall hCreateLuaInterface(void *_this, uchar stateType, bool renew)
{
	void *state = CreateLuaInterfaceFn(sharedHooker->getold(CREATELUAINTERFACE))(_this, stateType, renew);

	MENU->PushSpecial(Lua::SPECIAL_GLOB);
	MENU->GetField(-1, "hook");
		MENU->GetField(-1, "Call");
			MENU->PushNil();
			MENU->PushString("ClientStateCreated");
		MENU->Call(2, 0);
	MENU->Pop(2);

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
	void RunStringEx(const char* fileName, const char* path, const char* str, bool run = true, bool showErrors = true, bool pushErrors = true, bool noReturns = true)
	{
		return get<void(__thiscall*)(void*, char const*, char const*, char const*, bool, bool, bool, bool)>(RUNSTRINGEX)(this, fileName, path, str, run, showErrors, pushErrors, noReturns); //free cookies for people that know how to detect stuff
	}

};

LUA_FUNCTION(RunOnClient) {
	if (!clientState)
		LUA->ThrowError("Not in game");

	try {
		// cursed but ITS GONNA WORK
		const char* fileName = LUA->Top() > 2 && LUA->GetType(-3) == (int)GarrysMod::Lua::Type::String ? LUA->GetString(-3) : "";
		const char* path = LUA->Top() > 1 && LUA->GetType(-2) == (int)GarrysMod::Lua::Type::String ? LUA->GetString(-2) : "";
		const char* str = LUA->Top() > 0 && LUA->GetType(-1) == (int)GarrysMod::Lua::Type::String ? LUA->GetString(-1) : "";
		reinterpret_cast<CLuaInterface*>(clientState)->RunStringEx(fileName, path, str);
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
		MessageBoxA(NULL, "Can't get lua shared interface", "roc", NULL);

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