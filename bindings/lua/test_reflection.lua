local ptx = require('ptx_sugar')

print('--- PTX Reflection (Lua) Demo (sugar wrapper) ---')
local ok, classes = pcall(function() return ptx.list_classes() end)
if not ok or type(classes) ~= 'table' then
  print('ptx.list_classes unavailable or registry empty')
  classes = {}
end
print('Class count:', #classes)
for i, name in ipairs(classes) do
  if i <= 10 then
    print(string.format('  %02d %s', i, name))
  end
end

-- Attempt to construct RGBColor with 3 args (R,G,B)
-- Prefer sugar constructor via class proxy
local RGB = ptx.RGBColor
local c = nil
local okc, err = pcall(function() c = RGB(10,20,30) end)
if not okc or not c then
  print('RGBColor construction failed (maybe no 3-arg ctor reflected):', err)
else
  print('Constructed RGBColor: R=', c:get('R'), ' G=', c:get('G'), ' B=', c:get('B'))
  c:set('G', 200)
  print('After set G=200 -> G=', c:get('G'))
  -- Try invoking Add if present (expects one arg) then reading fields again
  local okm, merr = pcall(function() c:call('Add', 25) end)
  if okm then print('After Add(25): R=', c:get('R'), ' G=', c:get('G'), ' B=', c:get('B')) end
  if not okm then print('Method Add call failed:', merr) end
  -- Ergonomic sugar: direct field + method
  print('Sugar access R=', c.R, 'G=', c.G, 'B=', c.B)
  c.G = 123
  c:Add(10) -- direct method dispatch
  print('After sugar operations G=123 Add(10): R=', c.R, 'G=', c.G, 'B=', c.B)
end

-- Use class_info
local info = ptx.RGBColor:info()
if info then
  if type(info.fields) == 'table' then
    print('RGBColor fields:')
    for i,f in ipairs(info.fields) do
      print(string.format('  %s (size=%d type=%s)', f.name, f.size, f.type or '?'))
    end
  end
  if type(info.methods) == 'table' then
    print('RGBColor methods (first 5):')
    for i,m in ipairs(info.methods) do
      if i>5 then break end
      print(string.format('  %s argc=%d static=%s', m.name, m.argc, tostring(m.static)))
    end
  end
  -- Demonstrate call_sig if it can find a method with a signature and zero or one primitive arg
  if type(info.methods) == 'table' then
    for _, m in ipairs(info.methods) do
      if m.name == 'Add' and m.argc == 1 and m.signature and c then
        local okc, res = pcall(function() return c:call_sig('Add', m.signature, 5) end)
        print('call_sig Add via signature ->', okc, res)
        break
      end
    end
  end
  -- Attempt static signature-based call if any static method with zero args exists
  if type(info.methods) == 'table' then
    for _, m in ipairs(info.methods) do
      if m.static and m.argc == 0 and m.signature then
        local okc, res = pcall(function() return ptx.RGBColor:call_static(m.name, m.signature) end)
        print('call_static_sig', m.name, '->', okc, res)
        break
      end
    end
  end
end

-- Try signature-based constructor if signature exists
-- (Don't know exact signature string; just demonstrate API call fallback)
local ctor_sig_demo = nil -- user could fill with a real signature printed from info.constructors
if ctor_sig_demo then
  local okc, c2 = pcall(function() return ptx.RGBColor:new_sig(ctor_sig_demo) end)
  if okc and c2 then print('Constructed via signature:', ctor_sig_demo) end
end

print('--- Done ---')
