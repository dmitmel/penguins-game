# The Penguins Game!

A clone of the board game **"Hey, that's my fish!"** written in C and C++ for a university programming course.

![screenshot](docs/demo_screenshot.png)

## Rules

- The game is played on a board consisting of square tiles, which is randomly generated at the start of the game. Each tile is either an empty square with water, or an ice floe with one, two or three fish on it.
- Every player has a certain number of penguins (typically 2 or 3) that they use for playing the game.
- The game starts in the _placement phase_ where the players must place their penguins (in turns) on the board.
  - They can do it in whichever way they want, the only condition is that the penguins must be placed on unoccupied ice floes with a single fish.
  - When a penguin is placed, the fish on the tile it was placed on is automatically collected (this is explained below).
- Once all penguins have been placed, the _movement phase_ begins, in which the players, well, take turns to move their penguins.
  - Penguins can move either horizontally or vertically in a straight line over any number of ice tiles, however they can't jump over water or other penguins.
  - After a penguin is moved, the ice floe it was on "sinks" and becomes a water tile (consequently, it is removed from the board and other penguins can't step on it anymore) and the fish on the tile it has been moved onto is "eaten" by the penguin. For every collected fish one point is added to the player's score.
  - A penguin can become blocked by water tiles on all four sides, in which case it there are no possible moves for it (hint: you can use this to your advantage). If all penguins of some player are surrounded like that, this player's turn is skipped, but otherwise the player is obliged to make a move.
- The game ends when no player can make any moves. Whoever collects most fish wins.

## How to play

This project comes with two interfaces for playing the game - a text one and a graphical one.

### Graphical interface

Upon launching the application you will be greeted by the following screen:

![the start screen](docs/gui_start_screen.png)

To start playing you have to, obviously, click the "New Game" button. It will bring up the following window:

![new game dialog](docs/gui_new_game_dialog.png)

Here you can set the parameters of the game (the labels should be more or less self-descriptive). Most of them are set to default values which are good enough, but you must enter the names of the players before beginning. The drop-downs near the player name boxes lets you choose whether this particular player is a normal human or a bot program will be playing instead, allowing you to play against a computer program (yes, you can change both players to be bots to pit them against each other). After entering the parameters press "OK" to start the game.

![game screen](docs/gui_game_screen.png)

This is how the game screen looks (~~notice the 100% original sprites that definitely aren't based on any other game at all~~). The game, of course, starts in the placement phase. To place a penguin, simply select and click a desired tile using the mouse, the tiles penguins can be placed on will be highlighted.

![placement phase](docs/gui_placement.png)

When all penguins have been placed, the players can move their penguins either by selecting a penguin, clicking it, then selecting a destination tile and clicking that, or simply by selecting a penguin and dragging it to the desired tile. The tiles a penguin can be moved on also are highlighted.

![movement phase](docs/gui_movement.png)

The boxes on the top show the scores of the players, the current player is highlighted with the red outline. The application also displays hints in the bar at the bottom, for example by notifying when a move can't be performed.

![movement phase 2](docs/gui_movement_2.png)

The panel on the left records the performed moves, you can select one to view it (the "Back to the game" button will exit the viewer mode).

![move viewer](docs/gui_log_viewer.png)

That's it! Once the game ends, a table with the summary will be shown:

![game summary](docs/gui_summary.png)

### Text interface

Works in the terminal/console. Upon launching you will be asked to enter some parameters of the game:

![entering the game parameters](docs/tui_game_params.png)

The players will first be asked to enter coordinates for placing their penguins:

![placement phase](docs/tui_placement.png)

And then for the movement phase they are first asked for the coordinates of a penguin to move and then for the destination coordinates:

![movement phase](docs/tui_movement.png)

## Implementation overview

Strictly speaking, the task set by the course demanded writing only the terminal interface and the bot (more on that later) in plain C. Nevertheless, it was decided to implement the graphical interface for street cred and bragging rights, and also most effort was spent on it, hence it has more features than the text one. However, as we now had to work with three different interfaces - the interactive-mode graphical and terminal interfaces plus the autonomous-mode interface for the bot (again, more on that later), this necessitated some architectural considerations.

The project is divided into three main components: a standalone app for the graphical interface, a command-line program containing the terminal interface together with the autonomous mode commands, and a library shared between both, which implements the game logic. To follow the formal requirements, the library and the terminal interface are written in C99 without the use of any external libraries, and since there were no constraints with regards to the GUI, it is written in C++11 using the library [wxWidgets](https://www.wxwidgets.org). Both the GUI and the TUI are cross-platform and work on Linux, Windows and macOS. We are also using the library [munit](https://nemequ.github.io/munit) for unit testing the game logic library.

Another notable component is the bot for automatically playing the game. The algorithm of the bot itself, which considers what move to make given a state of the game, is implemented in the common logic library, and therefore is usable by the user interfaces to let the player compete against the computer. However, in the course task the bot served a different purpose: basically, there was a program which would pit the bot algorithms written by students against each other in a competition. This program would invoke the bot programs in turns, giving them a file with the game state in a simple machine-readable format (simple enough to be parsed with `scanf`). The bot was then supposed parse the command-line arguments because some data like the game phase wasn't stored in the state file, then load the state by reading the input file, evaluate and make a move, and finally write to the output file the new game state in the same format - this machine interface is the so-called the autonomous mode. Unfortunately the competition program itself isn't available since it is closed source and was created by the university, but really it doesn't do much more than just invoke our programs in a loop and render the board to the screen, so it _could_ be more or less trivially reimplemented, but honestly I didn't want to bother.

## Building and running the code

The project is built using [CMake](https://cmake.org) which has integrations with pretty much every IDE nowadays, so you can just clone the repository, import the project into your favorite IDE, and run any of the targets `penguins`, `penguins-gui` or `penguins-tests`, also typically IDEs allow changing the build options and editing the CMake cache. For UNIX systems (i.e. Linux and macOS) a Makefile wrapper is provided for convenience, using it is very straightforward:

```bash
# NOTE: It is recommended to use the flag -j8 when running Make to utilize
# multiple CPU cores for compilation (replace the 8 with the appropriate number
# of parallel processes for your system).

# To compile everything:
make
# To change the build directory (the default is 'build'):
make BUILD_DIR=something
# Some options are exposed as variables in the Makefile, others can be passed
# through CMAKE_EXTRA_FLAGS
make CMAKE_BUILD_TYPE=Release CMAKE_EXTRA_FLAGS='-DBUILD_TESTS=OFF'
# Ninja can be used as the build runner instead of Make
make CMAKE_GENERATOR=Ninja
# To re-generate the build files, e.g. when changing the CMake options
# (normally this happens automatically)
make cmake

make build        # compiles the TUI
make build-tests  # compiles the tests
make build-gui    # compiles the GUI
make run          # compiles and runs the TUI
make test         # compiles and runs the tests
make run-gui      # compiles and runs the GUI

# If it is necessary to run any of the executables with command-line arguments:
make build && build/penguins some=option something=else
make build-tests && build/penguins-tests --help
# Or in GDB:
make build-gui && gdb build/penguins-gui
```

It is also possible to just invoke CMake directly from the command-line on any platform:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

If you are using Visual Studio, there exist two ways of opening the project in it. You can either import it as a CMake project (`File > Open > CMake...` or just open the folder with the project, the `CMakeLists.txt` file will be automatically detected) or generate Visual Studio solution files with CMake and open those:

```bash
cmake -B build -G "Visual Studio 17 2022"
# Or any other VS version, check cmake --help for the list of available generators.
```

Apparently, doing it with the second method doesn't make VS generate as much temporary files and thus reduces the disk usage, however, it the integration with CMake becomes much less shallow (e.g. you have to remember to regenerate the project manually each time after editing the build script), so it is unclear which method we can recommend.

### Compilation of the GUI and wxWidgets

If wxWidgets can't be found on your system, the GUI simply won't be available, but otherwise the project can be compiled just fine. Installing wx, however, sometimes isn't exactly trivial. Generally, you can either use a pre-built version of wxWidgets (installed with a package manager, for example) or download and compile it from source on your system. The latter is relatively simple, all you have to do is enable the `BUILD_WXWIDGETS_FROM_SOURCE` option, like this:

```bash
cmake -B build -DBUILD_WXWIDGETS_FROM_SOURCE=ON
# When using the Makefile wrapper:
make BUILD_WXWIDGETS_FROM_SOURCE=ON
```

The CMake script will automatically download the source code of wxWidgets and compile it for you when building the project. This, however, requires a few hundreds of megabytes of disk space (usually under a gigabyte, something like 500-700 MBs) and a couple minutes to compile everything. Generally, to reduce the compilation time and disk usage a lot of components of wx are switched off in the script [wxwidgets_config.cmake](cmake/wxwidgets_config.cmake) to produce a minimal build only with the stuff that our application actually uses. The disk usage can be reduced even further by roughly a half with `wxBUILD_PRECOMP=OFF`, though it will double the time needed to compile wx.

The way of obtaining a pre-built version of wx (and its dependencies) is specific to your OS.

**Windows**

Well, on Windows you have to jump through some hoops to get wx to work. First of all, you'll have to use MSVC (the compiler included with Visual Studio). It is possible to compile the app using MinGW, however, it generates executables that can't be started due to some linking errors related to unresolved symbols within the standard library (don't ask; note though, that if you want to try to get it to work, you have to compile with `wxBUILD_PRECOMP=OFF`). Furthermore, MSVC is much faster than MinGW, at least in the case of our application, so even building wxWidgets from source doesn't take much time. Anyway, to download the wx libraries, go to <https://wxwidgets.org/downloads/> and in the block of the latest stable release find and click the button "Download Windows Binaries". Select the section appropriate for your Visual Studio version, and download the header files and development files and release DLLs for 64-bit systems. Unpack all three archives into a directory (replacing all files), say, `C:/wx/3.2.1`, create an environment called `WXWIN` and set it to that directory for automatic detection of the wx installation by CMake.

**Ubuntu/Debian**

```bash
# Currently the versions of wx libraries in the official repositories are too
# old for our app, so you'll have to add third-party repos with more up-to-date
# versions:
sudo apt-key adv --fetch-keys 'https://repos.codelite.org/CodeLite.asc'
sudo apt-add-repository 'deb https://repos.codelite.org/wx3.2/debian/ bullseye libs' # for Debian 11
sudo apt-add-repository 'deb https://repos.codelite.org/wx3.2/ubuntu/ focal universe' # for Ubuntu 20.04
sudo apt-add-repository 'deb https://repos.codelite.org/wx3.2/ubuntu/ jammy universe' # for Ubuntu 22.04
sudo apt-add-repository 'deb https://repos.codelite.org/wx3.2/ubuntu/ kinetic universe' # for Ubuntu 22.10
# And so on and so on, you get it, you must put the distro codename in the repo line.
sudo apt-get update
# Now, install wx and other packages required for development:
sudo apt-get install libwxgtk3.2unofficial-dev libgtk-3-dev libnotify-dev

# Alternatively, once the repositories catch up to v3.2, you can simply install
# it like this:
sudo apt-get install libwxgtk3.2-dev libgtk-3-dev
```

**Arch Linux**

```bash
sudo pacman -S wxwidgets
# That's it, fortunately at least here this is very easy.
```

**macOS**

```bash
brew install wxwidgets
```

Additionally, if you have installed wxWidgets in a non-standard location, or downloaded the source code yourself and compiled it in some other directory (or you want to do that, see the [platform-specific documentation for wx ports](https://docs.wxwidgets.org/trunk/page_port.html), the [page on building wx with CMake](https://docs.wxwidgets.org/trunk/overview_cmake.html), and the [page on building wx on Windows](https://docs.wxwidgets.org/trunk/plat_msw_install.html)), when building this project you can point CMake to the location of your installation of wx by setting the CMake variable `wxWidgets_ROOT_DIR` (or the environment variable `WXWIN`) to the installation directory on Windows or `wxWidgets_CONFIG_EXECUTABLE` (or the environment variable `WX_CONFIG`) to the location of the `wx-config` program within the installation on other OSes, like this:

```bash
cmake -B build -G "Visual Studio 17 2022" -DwxWidgets_ROOT_DIR="C:/wx/3.2.1"  # on Windows
cmake -B build -DwxWidgets_CONFIG_EXECUTABLE="/home/abc/wx/3.2.1/wx-config"   # on Linux
make CMAKE_EXTRA_FLAGS='-DwxWidgets_CONFIG_EXECUTABLE=/blah/blah/blah/wx-config' # with the Makefile wrapper
```

It should also be noted though that to get a static executable for the GUI (that is, a standalone executable that doesn't require DLLs of wxWidgets to be installed or shipped alongside it) you must compile wx from source. If you are doing it in a separate directory as explained in the paragraph above, you will need to set `BUILD_SHARED_LIBS` and `wxBUILD_SHARED` to `OFF` when building wx itself.

### Running the tests

To just check that everything works you can simply run `make test` as explained above, however, the test runner, munit, also has its own [command-line interface](https://nemequ.github.io/munit/#running-tests) with options, for example, for filtering test cases. For tests that depend on random values and randomly fail on some of them, munit offers a way of running the test cases with a specified seed to reproduce the issues. Each time the tests are run, the PRNG seed is printed, which you can then reuse like this:

```bash
make build-tests && build/penguins-tests --seed 0xdeadbeef
```