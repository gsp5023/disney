-------------------------------------------------------------------------------
-- gmake2.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
--------------------------------------------------------------------------------

-- Fix the gmake2 premake action to support "utility" projects
-- in the way we use them

local p = require("premake")
local gmake2 = require('gmake2')

local action = p.action._list["gmake2"]
p.override(action, "onProject", function(base, prj)
  if (prj.kind == p.UTILITY) then
      local makefile = p.modules.gmake2.getmakefilename(prj, true)
      p.generate(prj, makefile, p.modules.gmake2.cpp.generate)
  else
      base(prj)
  end
end)

local cpp = gmake2.cpp

cpp.outputFileset = function(cfg, kind, file)
--print(cfg.kind.."->"..file.." -> "..kind)
  if cfg.kind ~= p.UTILITY then
    _x('%s += %s', kind, file)
  end
end
