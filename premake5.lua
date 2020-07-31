workspace "cengine"
  configurations {"Debug", "Release"}
  location "build"

  filter "configurations:Debug"
    defines {"DEBUG"}
    symbols "On"

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
        if _TARGET_OS == "windows" then
          os.execute("IF EXIST bin ( RMDIR /S /Q bin )")
          os.execute("IF EXIST obj ( RMDIR /S /Q obj )")
          os.execute("IF EXIST build ( RMDIR /S /Q build )")
        else
          os.execute("rm -rf bin obj build")
        end
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
          if _TARGET_OS == "windows" then
            os.execute("xcopy /Q /E /Y /I res bin\\res")
          else
            os.execute("mkdir -p bin/res")
            os.execute("cp -rf res bin/res")
          end
        end
      end
  }

project "cppvoxel"
  kind "ConsoleApp"

  language "C++"
  targetdir "bin"
  objdir "obj"

  files {"src/**.cpp"}

  includedirs {"../cppgl/include", "../cppgl/vendors", "../cppgl/vendors/glm", "include"}
  libdirs "../cppgl/bin"
  links {"cppgl"}
  defines {"GLEW_STATIC"}

  staticruntime "On"
  flags {"LinkTimeOptimization"}
  linkoptions {"-static", "-static-libgcc", "-static-libstdc++"}

  filter {"system:windows"}
    libdirs "../cppgl/lib"
    links {"glew32s", "glfw3", "gdi32", "opengl32"}

  filter {"system:not windows"}
    links {"GLEW", "glfw", "rt", "m", "dl", "GL"}
