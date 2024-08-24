# Space Invaders
This repository will house my attempt to develop a clone of Space Invaders in C++ with the help of GLEW and GLFW
GLEW to support runtime acceleration and cross-platform compatibility, while GLFW will help to render the Graphical user interface and events.

Some goals for creating Space Invaders:
- Plan, organize, and execute till completion of Space Invaders and keeping the game's core functionalities
- Have a cross-platform compatible graphical interface that is easy to learn and use
- Design and develop player, bullet, and alien sprites and further add behaviours to each
- Have no memory leaks, when constructing a buffer system that handles all the game components

### Note: I moved this project from [here](https://github.com/Edison-Wei/Pong) with commits from March 17th - March 20th

## Setup
1. To begin, download or clone this repository on your local machine. (I am using a Mac with homebrew to keep packages structured)

2. Check you have clang/g++ installed on your system. Try ``` g++ --version ``` in the terminal to check.
If not, you can download it from the [clang website](https://releases.llvm.org/download.html). Any version will work for the application to run correctly.

 Note: If running on a Mac OS, clang will be installed already.

3. Then, go to the [GLEW website](https://glew.sourceforge.net/) and download version 2.1.0 of GLEW.
For GLFW, If you have homebrew on your system then
 ```command
 brew install glfw
 ```
 It will download the latest version '3.4'.\
 If not, you have to download from the [GLFW website](https://www.glfw.org/download) and make changes to the makefile.

4. Once all libraries are installed, change inside the 'Space Invaders' directory. Compile and run the application with the following command:
 ```
 make all
 ```

 If you did not use homebrew, you will need to make changes to
 ``` 
 -I/opt/homebrew/Cellar/glfw/3.4/include 
 -Iglew-2.2.0/include
 ```
 Both paths can start at the home directory and end at 'include.' Once done, you can compile as normal.

5. You are all set. Enjoy the game!!