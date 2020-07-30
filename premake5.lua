workspace "cengine"
  configurations {"Debug", "Release"}

  filter "configurations:Debug"
    defines {"DEBUG"}
    symbols "On"

  filter "configurations:Release"
    defines {"NDEBUG"}
    optimize "Speed"

  filter {}

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
          os.execute("mingw32-make -C build")
        else
          os.execute("make -C build")
        end
      end
  }

project "cppvoxel"
  kind "ConsoleApp"

  language "C++"
  location "build"
  targetdir "bin"
  objdir "obj"

  files {"src/**.cpp"}

  includedirs {"../cppgl/include", "../cppgl/vendors", "../cppgl/vendors/glm", "include"}
  libdirs "../cppgl/bin"
  links {"cppgl"}
  defines {"GLEW_STATIC"}

  filter {"system:windows"}
    libdirs "../cppgl/lib"
    links {"glew32s", "glfw3", "gdi32", "opengl32"}

  filter {"system:not windows"}
    links {"GLEW", "glfw", "rt", "m", "dl", "GL"}
