local cwd = os.getcwd()
local m5_root = "../../.."

local settings = {
    name = "submodule_example",
    root = nil,
    generators = {
        projects = function()
            return {
                function()
                    project ("acme_app_runner", cwd)
                        kind "consoleapp"

                        includedirs { m5_root }
                        files "src/main.c"

                        links {
                            "steamboat",
                            "runtime",
                            "renderer",
                            "log",
                            "cncbus"
                        }
                end
            }
        end
    }
}

-- Import the m5 premake project using a relative path
local m5 = require(m5_root.."/premake/project")(settings)
