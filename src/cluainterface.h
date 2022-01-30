class CLuaInterface
{
private:
	template<typename T>
	inline T get(unsigned short which)
	{
		return T((*(char***)(this))[which]);
	}
};