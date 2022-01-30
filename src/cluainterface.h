class CLuaInterface
{
private:
	template<typename t>
	inline t get(unsigned short which)
	{
		return t((*(char***)(this))[which]);
	}
public:
	bool RunStringEx(const char* fileName, const char* path, const char* str, bool run = true, bool showErrors = true, bool pushErrors = true, bool noReturns = true);
};