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
    ["trigger"] = "clean",
    ["description"] = "Delete generated project and build files",
    ["onStart"] =
      function()
        print "Cleaning files..."
      end,
    ["execute"] =
      function()
        os.rmdir("bin")
        os.rmdir("obj")
        os.rmdir("build")
      end,
    ["onEnd"] =
      function()
        print "Done."
      end
  }

  newaction {
    ["trigger"] = "build",
    ["description"] = "Build",
    ["execute"] =
      function()
        os.execute("premake5 gmake2")

        print "Embeding resources..."
        os.mkdir("res/embed")
        res = os.matchfiles("res/*.png")
        for _, resFile in ipairs(res) do
          resFile = string.sub(resFile, string.find(resFile, "/[^/]*$") + 1, string.find(resFile, ".[^.]*$") - 1)
          if _TARGET_OS == "windows" then
            os.execute("bin\\tools\\embed_images.exe "..resFile)
          else
            os.execute("./bin/tools/embed_images "..resFile)
          end
        end

        if _TARGET_OS == "windows" then
          if _OPTIONS["release"] then
            os.execute("mingw32-make -C build config=release")
          else
            os.execute("mingw32-make -C build")
          end
        else
          if _OPTIONS["release"] then
            os.execute("make -C build config=release")
          else
            os.execute("make -C build")
          end
        end
      end
  }

project "embed_images"
  targetdir "bin/tools"

  files {"tools/embed_images.cpp"}

project "cppvoxel"
  files {"src/**.cpp"}

  includedirs {"../cppgl/vendors", "../cppgl/vendors/glm", "include", "res/embed"}
  defines {"GLEW_STATIC"}

  filter {"system:windows"}
    libdirs "../cppgl/lib"
    links {"glew32s", "glfw3", "gdi32", "opengl32"}

  filter {"system:not windows"}
    links {"GLEW", "glfw", "rt", "m", "dl", "GL"}

  filter {}
