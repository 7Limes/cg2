# g2

A minimal ISA with builtin graphics designed to be simple to implement.

## General Info

- Single contiguous array of 32 bit signed integers
    - This array is zeroed on program start
    - Minimum size of 32 slots
- No registers
- `ldi` used to load immediate values, all other instructions refer directly to memory slots
- Includes arbitrary size 24 bit color window that updates 60 times per second
- The program is executed once per frame starting at index 0 and ending when the program counter goes past the last instruction
- On program start, external data can be loaded into program memory starting at a specific address (see [Binary Format](#binary-format))

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
- `3`: Control1 key (`0` = unpressed, `1` = pressed)
- `4`: Control2 key (`0` = unpressed, `1` = pressed) 
- `5`: A key (`0` = unpressed, `1` = pressed)
- `6`: B key (`0` = unpressed, `1` = pressed)
- `7`: Up key (`0` = unpressed, `1` = pressed)
- `8`: Down key (`0` = unpressed, `1` = pressed)
- `9`: Left key (`0` = unpressed, `1` = pressed)
- `10`: Right key (`0` = unpressed, `1` = pressed)
- `11`: Deltatime (ms)
- `12-13`: Unused
- `14`: Pseudoinstruction scratch register
- `15`: Call stack pointer (initialized to `16`)
- `16-31`: Call stack

## Instructions

> **Note**: Dollar signs (`$`) are used to indicate memory references. For example, if `a = 5` and `b = 7`, then `$a = $b` means that the value stored at address `7` will be copied to address `5`.

- `ldi dest, value`               $dest = value (Load immediate)
- `rdi dest, src`                 $dest = $$src (Read indirect)
- `sti dest, src`                 $$dest = $src (Store indirect)

- `add dest, a, b`                $dest = $a + $b
- `mul dest, a, b`                $dest = $a * $b
- `div dest, a, b`                $dest = $a / $b (Truncates towards negative infinity)
- `mod dest, a, b`                $dest = $a % $b (Keeps sign of denominator)

- `cmp dest, a, b`                $dest =  1 if $a < $b
                                           0 if $a == $b
                                          -1 if $a > $b

- `jne index, a, b`               Jump to $index if $a != $b  (Jump if not equal)

- `col r, g, b`                   Set the current color to ($r, $g, $b)
- `pix x, y`                      Draw a pixel at ($x, $y)


## Binary Format

g2 binary files have the extension `.g2b` and are formatted according to the `Program` struct in the following C code:

```c
struct Program {
    Header header;
    Instruction instructions[]
    uint32_t instruction_count;        // The number of instructions in the program
    uint32_t data_start_address;       // The address in memory to start loading data to
    uint32_t data_length;              // The number of data values
    int32_t data_values[data_length];  // The data values
}

struct Header {
    uint16_t signature;   // "g2"
    int32_t memory_size;  // The amount of memory for the program
    int32_t width;        // The width of the program window
    int32_t height;       // The height of the program window
}

struct Instruction {
    uint8_t opcode;
    int32_t args[3]
}
```

> **Note**: This format is documented in C structs for illustrative purposes only. The actual implementation need not use this exact code.
