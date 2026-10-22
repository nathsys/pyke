# Pyke

[![CI](https://github.com/nathsys/pyke/actions/workflows/ci.yml/badge.svg)](https://github.com/nathsys/pyke/actions/workflows/ci.yml)

Pyke is a chess move generator written in C++20. It's built for [perft](https://www.chessprogramming.org/Perft), which means counting every legal position reachable from a starting position up to a given depth. It's single-threaded, and almost all of the work happens at compile time.

On a Ryzen 9 7900X it counts perft 7 from the start position (3,195,901,860 nodes) in about 1.7 seconds, which works out to roughly 1.9 billion nodes per second. Perft 8 (84,998,978,956 nodes) takes about 47 seconds.

## How it works

- The board is stored as bitboards, one 64-bit integer per piece type and color.
- Sliding piece attacks come from lookup tables indexed with the BMI2 `pext` instruction.
- Only legal moves are generated. Before generating moves, Pyke builds a check mask and pin masks for the side to move, so it never has to make a move just to see if it leaves the king in check. King moves and en passant are the exceptions.
- The side to move, the remaining depth and en passant availability are template parameters. Each combination compiles into its own specialized function, so those branches disappear at runtime.
- Castling rights are a 4-bit field. King moves, rook moves and captures AND it with a mask from a 64-entry table, which handles moved kings, moved rooks and captured rooks in one step.
- At the last ply, moves are counted with `popcount` and never actually played.
- Release builds use profile-guided optimization. CMake first builds an instrumented binary, runs it to collect a profile, and then rebuilds using that profile.

## Building

You need CMake 3.21+, a recent GCC or Clang, and an x86-64 CPU with BMI2.

```
mkdir build
cd build
cmake ..
make release
```

`make release` builds an instrumented binary, runs perft 6 and 7 with it to collect the profile, and then builds the final `pyke` binary. Running both depths matters because the depth is a template parameter: perft 7 alone never runs the functions for black to move at odd depths, and perft 8 would then be about 25% slower. Use `make debug` for an unoptimized build with debug symbols.

## Usage

```
./pyke [depth] [moves...]
```

The depth defaults to 7 and can be anywhere from 1 to 10. You can also pass moves in coordinate notation, and they're played from the start position before counting starts. Castling is written as the king's move (`e1g1`) and promotions get the piece as a fifth letter (`e7e8q`).

```
./pyke 6
./pyke 5 e2e4 e7e5
```

The output lists the node count for each root move (a "divide"), followed by the total, the time and the speed.

## Correctness

Perft from the start position matches the published numbers up to depth 9 (2,439,530,234,167 nodes). Further depths have not been tested.

`make check` builds and runs the tests: perft 1 to 6 from the start position, plus 13 positions reached through move sequences that cover castling, en passant, pins and promotions.

## Limitations

- There's no FEN input yet. The search always starts from the initial position, with any moves you pass played first.
