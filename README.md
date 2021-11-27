gm_roc
======

This version of gm_roc aims to support the x64 version of gmod.

Brings the cool little function RunOnClient back into the menu state!

You should install [menu plugins](https://github.com/glua/gmod-menu-plugins) to get this to load properly (you can probs do it yourself though)

Then copy the lua folder to your gmod/gmod directory.

## Building the project
1) Get [premake](https://github.com/premake/premake-core/releases/download/v5.0.0-alpha14/premake-5.0.0-alpha14-linux.tar.gz) add it to your `PATH`
2) Get [garrysmod_common](https://github.com/danielga/garrysmod_common) (with `git clone https://github.com/danielga/garrysmod_common --recursive --branch=x86-64-support-sourcesdk`) and set an env var called `GARRYSMOD_COMMON` to the path of the local repo
3) Run `premake5 gmake --gmcommon=$GARRYSMOD_COMMON` in your local copy of **this** repo
4) Navigate to the makefile directory (`cd /projects/linux/gmake` or `cd /projects/macosx/gmake`)
5) Run `make config=releasewithsymbols_x86_64`
