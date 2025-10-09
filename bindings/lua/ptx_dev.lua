-- ptx_dev.lua
-- Dev loader that prepends the build directory to package.cpath
-- (require the native 'ptx' module without copying ptx.so.)
-- Usage:
--   local ptx = require('ptx_dev')
--   -- optional sugar:
--   local sugar = require('ptx_sugar') -- if you want the high-level proxies
-- Environment overrides:
--   PTX_LUA_BUILD_DIR  : custom absolute/relative path to directory containing ptx.so
--   PTX_LUA_NATIVE_NAME: override module name (default 'ptx')
-- Falls back gracefully if already on the path.

local loader, err = package.loadlib('../../build/ptx.so', 'luaopen_' .. 'ptx')
if loader then
    local m = loader()
    return m
end

error(string.format('[ptx_dev] failed to load native module, tried %s -> %s', 'ptx', 'build', tostring(err)))