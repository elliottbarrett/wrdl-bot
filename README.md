# Wordle bot

This is a Wordle solver that was initially built in *Node.js*, but was ported to *C++* as an exercise (and for performance!)

I intended to keep it private, but have made it public as part of a demonstration on using GitHub for Hack Frost NL 2022.

## Compiling the C++

I was too tired to make a `makefile`, so if you'd like to compile the *C++* (on a Mac), run the following command...

`clang++ -Wall -std=c++11 -O2 cpp-wordle.cpp -o cpp-wordle`

## Usage

Both the *C++* and *Node.js* programs accept the same arguments.

Either execute `./[program-name]` to run the bot against all possible Wordle puzzles.

Or run `./[program-name] [TARGET]` to see the bot solve for a given target!