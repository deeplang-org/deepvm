# DeepVM test suite

The tests are organized to mirror the WebAssembly 1.0 (MVP) specification: each
section of the binary format (Type / Import / Func / Table / Memory / Global /
Export / Start / Elem / Code / Data) has a dedicated category directory, and
every `.wasm` case has a one-to-one `.wat` counterpart (same basename, `.wat`
extension).

## Layout

| Directory | Covered section / feature                          |
| --------- | --------------------------------------------------- |
| `math/`   | i32/i64/f32/f64 arithmetic, comparison, conversion   |
| `shift/`  | i32/i64 `shl`/`shr_s`/`shr_u`/`rotl`/`rotr`          |
| `memory/` | narrow `load`/`store`, `memory.size`, `memory.grow`  |
| `global/` | Global section, `global.get`/`global.set`            |
| `table/`  | Table + Elem section, `call_indirect`                |
| `start/`  | Start section                                        |
| `control/`| `block`/`loop`/`if`/`br`/`br_if`/`br_table`/`select` |
| `builtin/`| imports of native `puts`/`puti`/`putf`/`putl`/`putd` |

## One-to-one wasm <-> wat

Every `.wasm` case has a sibling `.wat` that assembles to an equivalent binary:

```text
math/add_int_0_-10.wasm   <->  math/add_int_0_-10.wat
```

`test.py` runs each `.wasm` directly, then assembles its `.wat` with
`deepvm -o <tmp>.wasm <file>.wat` and runs the result, asserting the same output.
This round-trip verifies that the WAT assembler (`src/deep_wat.c`) and the wasm
loader/interpreter agree.

Every `.wat` case also has a `.wasm` sibling. A few of them exercise WAT-only
syntax (folded vs. flat instruction layout, `start`, `elem`, etc.):

- `math/add_folded`, `math/mul_flat`, `math/i64_const`
- `control/if_else`, `control/loop_sum`
- `global/global_get`
- `memory/memory_store_load`
- `table/call_indirect`
- `start/start_section`
- `builtin/puts_string`

## Run

```shell
cd test
python3 test.py
```
