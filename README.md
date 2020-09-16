# glpong
Simple 2D implementation of Pong using OpenGL in C++.
Follows [this](https://www.youtube.com/playlist?list=PLysLvOneEETMnZWk6fVusftDHV9duoazP) playlist on YouTube.

## Required Installations/Setup

These instructions are configured for Visual Studio but should be similar to other IDEs.

#### Some Pointers
* Make sure everything you configure is set for **_x64_** in your IDE.
* *$(SolutionDir)* is the directory containing the *.sln* file for the Visual Studio Solution
* *$(ProjectDir)* is the directory containing the *.vcxproj* file for the Visual Studio Project

### Initial
1. Create project in IDE (These directions are for Visual Studio 2019)
2. Create directory named 'Linking' in *$(SolutionDir)
3. Set Project Properties
    * Linker -> Input
        * Additional Dependencies
            \+ opengl32.lib
    * VC++ Directories
        * Library Directories
            \+ $(SolutionDir)\Linking\lib;
        * Include Directories
            \+ $(SolutionDir)\Linking\include;
4. Create directories *src*, *lib*, and *assets* in *$(ProjectDir)*

### GLFW
1. Download pre-compiled binaries from [GLFW](https://www.glfw.org/download.html)
2. Unzip download and add files to linking directory
    * *GLFW\include* -> *$(SolutionDir)\Linking\include*
    * all *.lib* files from corresponding VS folder -> *Linking\lib\GLFW*
3. Add files to project directory
    * *glfw3.dll* from corresponding VS folder -> *$(ProjectDir)*
4. Set Project Properties
    * Linker -> Input
        * Additional Dependencies
            \+ GLFW\glfw3.lib

### GLAD
1. Download package (*.zip* file) from [their site](https://glad.dav1d.de/)
    * Only change these default settings:
        * API -> gl: Version 3.3
        * Options -> Generate a Loader: checked **(required)**
2. Unzip download and add files to project
	* *glad\include* -> *$(SolutionDir)\Linking\include*
		* this should add two folders (*glad*, *KHR*) to your include directory
	* *glad\src\glad.c* -> *$(ProjectDir)\lib*
