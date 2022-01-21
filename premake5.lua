newoption({
	trigger = "gmcommon",
	description = "Sets the path to the garrysmod_common (https://github.com/danielga/garrysmod_common) directory",
	value = "path to garrysmod_common directory"
})

local gmcommon = assert(_OPTIONS.gmcommon or os.getenv("GARRYSMOD_COMMON"),
	"you didn't provide a path to your garrysmod_common (https://github.com/danielga/garrysmod_common) directory")
include(path.join(gmcommon, "generator.v3.lua"))

CreateWorkspace({name = "roc"})
	CreateProject({serverside = true, source_path = "src"})
		IncludeLuaShared()
		includedirs({"include"})
		files({
			"include/*.cpp",
			"include/*.h",
		})