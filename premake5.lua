workspace "cengine"
  configurations {"Debug", "Release"}
  architecture "x64"
  location "build"

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

        if _OPTIONS["release"] then
          print "Copying resources..."
          os.mkdir("bin/res")
          res = os.matchfiles("res/*")
          for _, resFile in ipairs(res) do
            printf("Copying %s", resFile)
            os.copyfile(resFile, "bin/"..resFile)
          end
        end
      end
  }

project "cppvoxel"
  language "C++"
  targetdir "bin"
  objdir "obj"

  files {"src/**.cpp"}

  includedirs {"../cppgl/include", "../cppgl/vendors", "../cppgl/vendors/glm", "include"}
  libdirs "../cppgl/bin"
  links {"cppgl"}
  defines {"GLEW_STATIC"}

  staticruntime "On"
  flags {"LinkTimeOptimization", "ShadowedVariables"}
  enablewarnings {"all"}
  buildoptions {"-fdiagnostics-color=always"}
  linkoptions {"-static", "-static-libgcc", "-static-libstdc++"}

  filter {"system:windows"}
    libdirs "../cppgl/lib"
    links {"glew32s", "glfw3", "gdi32", "opengl32"}

  filter {"system:not windows"}
    links {"GLEW", "glfw", "rt", "m", "dl", "GL"}

  filter "configurations:Debug"
    kind "ConsoleApp"
    defines {"DEBUG"}
    symbols "On"
    targetsuffix ".debug"
    buildoptions {"-g3", "-O0"}

  filter "configurations:Release"
    kind "WindowedApp"
    defines {"NDEBUG"}
    optimize "Speed"

  filter {}
