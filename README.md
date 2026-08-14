# g2

A minimal ISA with builtin graphics designed to be simple to implement.

## General Info

- Single contiguous array of 32 bit signed integers
    - This array is zeroed on program start
    - Minimum size of 16 slots
- No registers
- `ldi` used to load immediate values, all other instructions refer directly to memory slots
- Includes arbitrary size 24 bit color window that updates at 60 fps
- The program is executed once per frame starting at index 0 and ending when the program counter reaches the last instruction

## Procedure for each Frame

1. Calculate delta time
2. Update reserved memory
3. Execute program instructions
4. Present the framebuffer
5. Clear the framebuffer using the current color

## Reserved Memory

- `0`: Constant zero (`0`) value
- `1`: Constant one (`1`) value
- `2`: Constant negative one (`-1`) value
- `3`: Control1 key
- `4`: Control2 key
- `5`: A key
- `6`: B key
- `7`: Up key
- `8`: Down key
- `9`: Left key
- `10`: Right key
- `11`: Deltatime (ms)
- `12-13`: Unused
- `14`: Pseudoinstruction scratch register
- `15`: Call stack pointer (initialized to `16`)
- `16-31`: Call stack

## Instructions

- `ldi dest, value`               $dest = value (Load immediate)
- `rdi dest, src`                 $dest = $$src (Read indirect)
- `sti dest, src`                 $$dest = $src (Store indirect)

- `add dest, a, b`                $dest = $a + $b
- `mul dest, a, b`                $dest = $a * $b
- `div dest, a, b`                $dest = $a / $b (Truncates towards negative infinity)
- `mod dest, a, b`                $dest = $a % $b (Keeps sign of denominator)
                                  $rdest = $a % $b

- `cmp dest, a, b`                $dest =  1 if $a < $b
                                           0 if $a == $b
                                          -1 if $a > $b

- `jne index, a, b`               Jump to $index if $a != $b  (Jump if not equal)

- `col r, g, b`                   Set the current color to ($r, $g, $b)
- `pix x, y`                      Draw a pixel at ($x, $y)


## Pseudoinstructions

Instructions that are like macros for common combinations of other instructions.
These will be recognized and emitted by the assembler; they are not to be implemented by the interpreter.

- `cpy dest, src`               $dest = $src
    add dest, src, 0
- `sub dest, a, b`              $dest = $a - $b
    mul dest, 2, b
    add dest, a, dest

- `inc x`                       $x = $x + 1
    add x, x, 1

- `dec x`                       $x = $x - 1
    add x, x, 2

- `abs dest, a`                 $dest = abs($a)
    cmp dest, a, 0
    mul dest, dest, a
    mul dest, dest, 2

- `ja [label name], a, b`       Jump to the index of the given label if $a != $b
    ldi 14, [label index]
    jne 14, a, b

- `sne a, b`                    Skip the next instruction if $a != $b
    jne [index+2] a, b

- `call [label name]`           Jump to the given label and store return address on the call stack
    ldi 14 [index+2]
    sti 15, 14
    add 15, 15, 1
    ldi 14, [label index]
    jne 14, 0, 0

- `ret`                         Pop a return address off the call stack and jump to it
    rdi 14, 15
    add 15, 15, 2
    jne 14, 0, 0
