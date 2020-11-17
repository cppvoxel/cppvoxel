function getCmdOutput(cmd)
  local handle = io.popen(cmd)
  local output = handle:read("*a")
  handle:close()

  output = string.sub(output, 0, string.find(output, "\n") - 1)

  return output
end

function buildProject(project, forceRelease)
  local buildCommand = "make -C build "..project

  if _OPTIONS["release"] or forceRelease then
    buildCommand = buildCommand.." config=release"
  end

  if _TARGET_OS == "windows" then
    buildCommand = "mingw32-"..buildCommand;
  end

  os.execute(buildCommand)
end

function getToolBuildPath(name)
  if _TARGET_OS == "windows" then
    return "bin\\tools\\"..name..".exe "
  else
    return "./bin/tools/"..name
  end
end

workspace "cppvoxel"
  configurations {"Debug", "Release"}
  architecture "x64"
  location "build"

  kind "ConsoleApp"
  language "C++"
  cppdialect "C++11"
  targetdir "bin"
  objdir "obj"

  staticruntime "On"
  flags {"LinkTimeOptimization", "ShadowedVariables"}
  enablewarnings {"all"}
  buildoptions {"-fdiagnostics-color=always"}
  linkoptions {"-static", "-static-libgcc", "-static-libstdc++"}

  filter "configurations:Debug"
    defines {"DEBUG"}
    symbols "On"
    targetsuffix ".debug"
    buildoptions {"-g3", "-O0"}

  filter "configurations:Release"
    defines {"NDEBUG"}
    optimize "Speed"

  filter {}

  newoption {
    ["trigger"] = "release",
    ["description"] = "Release build"
  }

  newaction {
    ["trigger"] = "format",
    ["description"] = "Formats all source files",
    ["onStart"] = function()
      print "Formatting files..."
    end,
    ["execute"] = function()
      os.execute("astyle --options=.astylerc src/*.cpp tools/*.cpp include/*.h shaders/*.glsl")
    end,
    ["onEnd"] = function()
      print "Done."
    end
  }

  newaction {
    ["trigger"] = "clean",
    ["description"] = "Delete generated project and build files",
    ["onStart"] = function()
      print "Cleaning files..."
    end,
    ["execute"] = function()
      os.rmdir("bin")
      os.rmdir("obj")
      os.rmdir("build")
      os.rmdir("embed")
    end,
    ["onEnd"] = function()
      print "Done."
    end
  }

  newaction {
    ["trigger"] = "embed",
    ["description"] = "Embed resource files",
    ["execute"] = function ()
      os.execute("premake5 gmake2")
      buildProject("embed_images", true)
      buildProject("embed_shaders", true)

      print "Embeding resources..."
      os.mkdir("embed/res")
      res = os.matchfiles("res/*.png")
      for _, resFile in ipairs(res) do
        resFile = string.sub(resFile, string.find(resFile, "/[^/]*$") + 1, string.find(resFile, ".[^.]*$") - 1)
        os.execute(getToolBuildPath("embed_images").." "..resFile)
      end
      os.execute(getToolBuildPath("embed_shaders"))
    end
 }

  newaction {
    ["trigger"] = "build",
    ["description"] = "Build",
    ["execute"] = function()
      os.execute("premake5 gmake2")
      buildProject("cppvoxel")
    end
  }

project "embed_images"
  targetdir "bin/tools"
  files {"tools/embed_images.cpp"}

project "embed_shaders"
  targetdir "bin/tools"
  files {"tools/embed_shaders.cpp"}

project "cppvoxel"
  files {"src/**.cpp"}

  includedirs {"../cppgl/vendors", "../cppgl/vendors/glm", "include", "embed"}

  local git_hash = getCmdOutput("git rev-parse HEAD")
  local git_branch = getCmdOutput("git rev-parse --abbrev-ref HEAD")
  defines {"GLEW_STATIC", "GIT_HASH=\""..git_hash.."\"", "GIT_BRANCH=\""..git_branch.."\""}

  filter {"system:windows"}
    libdirs "../cppgl/lib"
    links {"glew32s", "glfw3", "gdi32", "opengl32"}

  filter {"system:not windows"}
    links {"GLEW", "glfw", "rt", "m", "dl", "GL"}

  filter {}
