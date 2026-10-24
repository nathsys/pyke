# Pyke

[![CI](https://github.com/nathsys/pyke/actions/workflows/ci.yml/badge.svg)](https://github.com/nathsys/pyke/actions/workflows/ci.yml)

Pyke is a chess move generator written in C++20. It's built for [perft](https://www.chessprogramming.org/Perft), which means counting every legal position reachable from a starting position up to a given depth. It's single-threaded, and almost all of the work happens at compile time.

On a Ryzen 9 7900X, from the start position:

| | Nodes | Without hashing | Nodes/s | With `--hash 1024` | Nodes/s |
|---|---|---|---|---|---|
| Perft 7 | 3,195,901,860 | 1.3 s | 2.4 billion | 0.34 s | 9.5 billion |
| Perft 8 | 84,998,978,956 | 36 s | 2.4 billion | 4.2 s | 20 billion |
| Perft 9 | 2,439,530,234,167 | 16.6 min | 2.4 billion | 57 s | 42.6 billion |
| Perft 10 | 69,352,859,712,417 | | | 14.5 min | 79.9 billion |

Without hashing every node is counted, which is about 2.4 billion nodes per second: the raw speed of the move generator. Hashing is off by default. When it's turned on, positions that repeat in the tree are looked up instead of counted again.

## Speed

- **Bitboards:** one 64-bit integer per piece type and color.
- **Templates:** side to move, remaining depth and en passant are template parameters, so their branches are resolved at compile time.
- **Forced inlining:** every helper is inlined; the only real calls are one per search node.
- **Legal moves only:** check and pin masks filter out illegal moves up front; only king moves and en passant are tested after being made.
- **PEXT:** sliding attacks come from tables indexed with the BMI2 `pext` instruction.
- **No moves made at the last ply:** pawns are counted with shifts, king moves by testing target squares directly.
- **Setwise AVX-512:** all sliders and knights are counted at once, eight directions per register. All rooks and queens are slid north together in one bitboard fill. Two of your own pieces can never reach the same square in the same direction, so one `popcount` of the result equals the sum of their individual move counts. A 512-bit register holds eight bitboards, so all eight directions are filled in parallel, and a second register does the eight knight jumps. The fill itself is the classic Kogge-Stone algorithm. What Pyke adds is running all eight directions in one AVX-512 register and summing the popcounts for bulk counting.
- **Early outs:** pin detection, pinned pawns and promotions are skipped when they can't apply.
- **Castling mask table:** one AND per move updates castling rights.
- **PGO:** profile-guided optimization, trained with both colors to move at every depth.

## Hashing

With `--hash MB`, Pyke keeps a transposition table of node counts, keyed by Zobrist hash and remaining depth.

- **Keys:** a fixed table of random 64-bit keys for every piece on every square, plus castling rights, en passant file, side to move and depth, generated at compile time with splitmix64.
- **Incremental updates:** each move XORs the keys of the squares it changes into the hash and XORs them out again afterwards. This only happens for moves whose children probe the table, so with hashing off there is almost no cost.
- **Probing from depth 2:** a depth-1 node is cheaper to count than to look up, so the table is only used from depth 2 up.
- **Prefetch:** the table slot depends only on the pieces, side to move and depth, so the parent can prefetch the child's slot before calling it.
- **Two-entry buckets:** each 32-byte bucket has one entry that keeps the deepest result and one that's always replaced, so deep subtrees survive the many shallow ones.

## Legality and edge cases

Every node starts by building a `MaskSet` for the side to move (`create_masks` in `maskset.hpp`). Moves are then legal by construction: each piece's attack bitboard is ANDed with the right mask, and nothing has to be made and undone to check it.

- **`targets`:** squares that are empty or hold an enemy piece, i.e. `~own`.
- **Finding pins and sliding checks:** Pyke looks up rook and bishop attacks from the king square, with only the enemy pieces as blockers. The rays pass through Pyke's own pieces and stop at the first enemy piece. ANDing that with the enemy sliders of the right type gives every candidate pinner and checker. For each one, `between_squares[king][slider]` is the line from the king up to and including the slider, and `popcount(line & own)` decides what it is:
  - **0:** it's a check, and the line is ORed into `check_mask`.
  - **1:** it's a pin, and the line is ORed into `pinmask_dg` or `pinmask_orth`.
  - **2 or more:** nothing.
- **Knight and pawn checks:** are detected when the move is made, from the moved piece's attacks, and handed to the next node as a `checker` bitboard. It's ORed into `check_mask`, and `checkers` is increased by its `popcount`, so even a combined knight-and-pawn check counts as a double check.
- **Check:** with one checker, `targets &= check_mask`, so every non-king move must capture the checker or block its line. With two or more, only king moves are generated.
- **Pins:** `nopin = ~(pinmask_dg | pinmask_orth)`. Unpinned pieces use `targets` as is. A pinned bishop, or a queen pinned diagonally, moves within `targets & pinmask_dg`; a pinned rook, or a queen pinned orthogonally, within `targets & pinmask_orth`. Pinned knights are skipped. Pinned pawns get `targets & pinmask_dg & occupied` (captures along the diagonal only) or `targets & pinmask_orth & empty` (pushes along the file only).
- **King moves:** the candidates are `king_attacks & targets`, without `check_mask`. Each one is tested with `is_attacked(square, occupied ^ king)`. Taking the king off the board lets sliders see through its old square, so it can't step back along a checking ray.
- **En passant:** the flag is only set when an enemy pawn stands next to the pushed pawn. The capture empties two squares on one rank, and the captured pawn may have been shielding a diagonal, so the masks can't cover it. Pyke makes the capture and tests its own king with `is_attacked`.

## Building

You need CMake 3.21+, a recent GCC or Clang, and an x86-64 CPU with BMI2. AVX-512 is used when the CPU has it.

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

The depth can be anywhere from 1 to 10. You can also pass moves in coordinate notation, and they're played from the start position before counting starts. Castling is written as the king's move (`e1g1`) and promotions get the piece as a fifth letter (`e7e8q`).

```
./pyke 6
./pyke 5 e2e4 e7e5
```

The output lists the node count for each root move (a "divide"), followed by the total, the time and the speed.

Run without arguments, Pyke speaks UCI, so tools that drive chess engines can use it. It supports `uci`, `isready`, `ucinewgame`, `position startpos|fen ... [moves ...]` and `go perft N`.

Add `--hash MB` to either mode to turn on the hash table, for example `./pyke 8 --hash 1024` or `./pyke --hash 256`.

## Correctness

Perft from the start position matches the published numbers up to depth 10 (69,352,859,712,417 nodes). Depth 10 was run with hashing; depths up to 9 were checked both with and without it.

Pyke also passes [perftcheck](https://grandchesstree.com/perftcheck) on its full corpus of about 140,000 positions, including its edge cases: all 561,641 cases at depths 1 to 4 and all 140,410 at depth 5. With hashing on, it passes all 561,641 cases at depths 1 to 4 as well.

`make check` builds and runs the tests: perft 1 to 6 from the start position, the standard test positions from the Chess Programming Wiki (Kiwipete and positions 3 to 6), and positions that cover castling, en passant, pins and promotions. Every case runs twice, once without and once with hashing.
