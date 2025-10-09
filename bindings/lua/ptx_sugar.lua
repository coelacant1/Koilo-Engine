-- High-level ergonomic sugar wrapper over the native 'ptx' module.
-- Usage:
--   local ptx = require('ptx_sugar')  -- returns wrapped module
--   local Color = ptx.RGBColor         -- class proxy
--   local c = Color(10,20,30)          -- invokes ptx.new('RGBColor', ...)
--   print(c.R, c.G, c.B)               -- field sugar from C binding
--   c.G = 42
--   c:Add(5)
--   local c2 = Color:new_sig('(unsigned char, unsigned char, unsigned char)', 1,2,3)
--   local info = Color:info()
--   for _,m in ipairs(info.methods) do print(m.name, m.argc) end
-- Static method call sugar:
--   Color.static.InterpolateColors(c, c2, 0.5)
--   -- Also available: Color:call_static('InterpolateColors', ...)
--
-- This layer keeps zero persistent per-instance overhead beyond one metatable per class proxy.

local native = require('ptx_dev')

local M = {}
setmetatable(M, { __index = native }) -- fallback to native functions

-- Cache of class proxy objects keyed by class name
local class_cache = {}

local class_mt = {}
class_mt.__index = class_mt

-- Make the proxy callable -> constructor by argcount
function class_mt:__call(...)
  return native.new(self.__name, ...)
end

-- Explicit signature-based construction
function class_mt:new_sig(signature, ...)
  return native.new_sig(self.__name, signature, ...)
end

-- Fetch (and cache) class info
function class_mt:info()
  if not self.__info then
    self.__info = native.class_info(self.__name) or {}
  end
  return self.__info
end

-- Static method namespace: proxy.static.MethodName(...) -> ptx.call_static
function class_mt:__index(k)
  if k == 'static' then
    -- build a static method dispatcher table lazily
    local static_tbl = {}
    local info = self:info()
    if info.methods then
      for _,m in ipairs(info.methods) do
        if m.static then
          static_tbl[m.name] = function(...)
            return native.call_static(self.__name, m.name, ...)
          end
        end
      end
    end
    self.static = static_tbl
    return static_tbl
  end
  return rawget(class_mt, k)
end

-- Allow calling static methods via proxy:Color:call_static(name,...)
function class_mt:call_static(method, ...)
  return native.call_static(self.__name, method, ...)
end

local function get_class_proxy(name)
  local p = class_cache[name]
  if p then return p end
  p = setmetatable({ __name = name }, class_mt)
  class_cache[name] = p
  return p
end

-- Top-level metamethod: access unknown keys as class proxies first
setmetatable(M, {
  __index = function(_, k)
    -- If native already has it (function like list_classes), return that
    local v = native[k]
    if v ~= nil then return v end
    -- Attempt to resolve as class name (cheap list scan once, then cached)
    -- Create a proxy even if class not present yet; instantiation will error clearly.
    return get_class_proxy(k)
  end
})

-- Pre-populate proxies for existing classes for convenience
local ok, classes = pcall(native.list_classes)
if ok and type(classes) == 'table' then
  for _,name in ipairs(classes) do
    if type(name) == 'string' and name ~= '' then
      get_class_proxy(name)
    end
  end
end

return M
