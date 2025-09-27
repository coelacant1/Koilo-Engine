local ptx = require('ptx')

print('--- PTX Reflection (Lua) Demo ---')
local classes = ptx.list_classes()
print('Class count:', #classes)
for i, name in ipairs(classes) do
  if i <= 10 then
    print(string.format('  %02d %s', i, name))
  end
end

-- Attempt to construct RGBColor with 3 args (R,G,B)
local c = ptx.new('RGBColor', 10, 20, 30)
if not c then
  print('RGBColor construction failed (maybe no 3-arg ctor reflected)')
else
  print('Constructed RGBColor: R=', c:get('R'), ' G=', c:get('G'), ' B=', c:get('B'))
  c:set('G', 200)
  print('After set G=200 -> G=', c:get('G'))
  -- Try invoking Add if present (expects one arg) then reading fields again
  local ok, err = pcall(function()
    c:call('Add', 25)
  end)
  if ok then
    print('After Add(25): R=', c:get('R'), ' G=', c:get('G'), ' B=', c:get('B'))
  else
    print('Method Add call failed:', err)
  end
  -- Ergonomic sugar: direct field + method
  print('Sugar access R=', c.R, 'G=', c.G, 'B=', c.B)
  c.G = 123
  c:Add(10) -- direct method dispatch
  print('After sugar operations G=123 Add(10): R=', c.R, 'G=', c.G, 'B=', c.B)
end

-- Use class_info
local info = ptx.class_info('RGBColor')
if info then
  print('RGBColor fields:')
  for i,f in ipairs(info.fields) do
    print(string.format('  %s (size=%d type=%s)', f.name, f.size, f.type or '?'))
  end
  print('RGBColor methods (first 5):')
  for i,m in ipairs(info.methods) do
    if i>5 then break end
    print(string.format('  %s argc=%d static=%s', m.name, m.argc, tostring(m.static)))
  end
  -- Demonstrate call_sig if we can find a method with a signature and zero or one primitive arg
  for _, m in ipairs(info.methods) do
    if m.name == 'Add' and m.argc == 1 and m.signature then
      local ok, res = pcall(function()
        return c:call_sig('Add', m.signature, 5)
      end)
      print('call_sig Add via signature ->', ok, res)
      break
    end
  end
  -- Attempt static signature-based call if any static method with zero args exists
  for _, m in ipairs(info.methods) do
    if m.static and m.argc == 0 and m.signature then
      local ok, res = pcall(function()
        return ptx.call_static_sig('RGBColor', m.name, m.signature)
      end)
      print('call_static_sig', m.name, '->', ok, res)
      break
    end
  end
end

-- Try signature-based constructor if signature exists
-- (We don't know exact signature string; just demonstrate API call fallback)
local ctor_sig_demo = nil -- user could fill with a real signature printed from info.constructors
if ctor_sig_demo then
  local c2 = ptx.new_sig('RGBColor', ctor_sig_demo)
  if c2 then print('Constructed via signature:', ctor_sig_demo) end
end

print('--- Done Extended ---')

print('--- Done ---')
