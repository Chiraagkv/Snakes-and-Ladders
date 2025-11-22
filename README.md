# **Snakes and Ladders (C & Raylib)**

A fully featured, responsive implementation of the classic board game Snakes and Ladders, built in C using the **Raylib** library for graphics and user interface.

## **🌟 Features**

* **Custom Board Generation:** Generates new, validated snake and ladder configurations on startup or restart.  
* **Multiplayer Support:** Supports 2 to 6 local players with customizable names.  
* **Smooth Animation:** Implements smooth movement animation using C-based easing functions (smoothstep).  
* **Persistent Saves:** Allows saving and loading game state to/from `s_and_l_save.txt`. Automatically prompts to resume a previous game on startup.

## **🏗 Project Structure**

The project follows a standard C organization, making it easy to navigate and compile using the provided Makefile.

| Directory/File | Purpose |
| :---- | :---- |
| `Makefile` | The build automation script (compiles, cleans, and runs the game). |
| `include/s_and_l.h` | Defines all global constants, data structures (Player, Snake, Ladder), external variables, and function prototypes. |
| `src/main.c` | Contains the main function, game state machine logic, and handles user input/state transitions. |
| `src/game.c` | Contains all core game logic: dice roll, S/L generation, file I/O (Save/Load), and animation state management. |
| `src/drawing.c` | Contains all graphical functions: drawing the board grid, S/L elements, player tokens, and UI panels. |

## **🚀 Building and Running**

This project uses the raylib library and is compiled via gcc using the provided Makefile.

### **Prerequisites**

You must have gcc and the raylib development library installed on your system. The Makefile relies on the linker flags required for raylib on Linux/macOS systems (-lraylib \-lm \-ldl \-lpthread \-lrt \-lX11).

### **Compilation and Execution**

Navigate to the root directory of the project (where the `Makefile` is located) in your terminal and use the following commands:

| Command | Action |
| :---- | :---- |
| `make` | **Builds the executable.** Compiles all .c files in `src/`, creates the necessary `build/` directory, and links the final executable named `snakes_and_ladders`. |
| `make run` | **Runs the game.** Executes the built program. This command implicitly calls make first if the executable is out of date or doesn't exist. |
| `make clean` | **Cleans up.** Removes the `build/` directory, the `snakes_and_ladders` executable, and any created save file (`s_and_l_save.txt`). |

### **Example Build Sequence**

```bash

# 1. Compile the project 
make

# 2. Start the game 
./snakes_and_ladders 
# OR: 
make run

# 3. Clean up the project files 
make clean

```
