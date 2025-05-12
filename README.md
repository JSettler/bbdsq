Welcome to bbDSQ, an implementation of Dou Shou Qi (Jungle chess) using bitboards & hashtables!


Build instructions (in Linux):

    1. unzip the archive to folder 'bbdsq' and copy "arial-monospace.ttf" (in bbdsq-folder) to "build"-folder (if it doesn't exist there yet)

    2. type in terminal: "CD bbdsq/build"

    3. type "cmake .."  (creates all makefiles, using default [release] model)  [optionally delete files from a previous build, before that command]

    4. type "make" to compile/link (build) the project into an executable (binary) file  [optionally "make clean && make" for a fresh build]


To select another thinking-level, start the program with a parameter, like so: "./bbdsq --depth 7"

To play first (by default, the computer starts) start the program with "--me" parameter

Start it with "-h" (or "--help") argument to get a list of all possible parameters.

Use "--ttsize <MB>" to start it with a non-default-sized transposition table (default: 256 MB)

Take back moves with <backspace>, undo takebacks with <shift>-<backspace>.

Program automatically saves game after quit and auto-loads it at start (if exists).



Have fun :)


-----


Reference:
https://en.wikipedia.org/wiki/Jungle_(board_game)


![Jungle board](https://github.com/JSettler/bbdsq/blob/master/bbdsq.png)


-----


social media:

https://discord.gg/257jDH3nmn  - our Dou Shou Qi/Jungle chess community


