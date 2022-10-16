add_rules("mode.debug", "mode.release")

set_languages("c++20")
add_requires("fmt")

target("FlagMod")
	set_default(true)
    set_kind("headeronly")
	add_includedirs("include")
	add_packages("fmt")

local examples = {}
for _, filepath in ipairs(os.files("examples/*.cpp")) do
	local target_name = path.basename(filepath)
	target(target_name)
		set_kind("binary")
		add_includedirs("include")
		add_files(filepath)
		add_packages("fmt")
	table.insert(examples, target_name)
end

target("examples")
	set_kind("phony")
	add_deps(unpack(examples))

-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- set warning all as error
--    set_warnings("all", "error")

--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")

--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--

