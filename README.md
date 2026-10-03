# g2

A minimal ISA with builtin graphics designed to be simple to implement.

## General Info

- Memory is a contiguous array of 32 bit signed integers
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

| Opcode | Syntax | Operation |
|--------|--------|-----------|
| `0x0` | `ldi dest, value` | $dest = value |
| `0x1` | `rdi dest, src` | $dest = $$src |
| `0x2` | `sti dest, src` | $$dest = $src |
| `0x3` | `add dest, a, b` | $dest = $a + $b |
| `0x4` | `mul dest, a, b` | $dest = $a * $b |
| `0x5` | `div dest, a, b` | $dest = $a / $b (truncates towards negative infinity) |
| `0x6` | `mod dest, a, b` | $dest = $a % $b (keeps sign of denominator) |
| `0x7` | `cmp dest, a, b` | $dest = 1 if $a < $b, 0 if $a == $b, -1 if $a > $b |
| `0x8` | `jne index, a, b` | Set program counter to $index if $a != $b |
| `0x9` | `col r, g, b` | Set current color to ($r, $g, $b), clamped to [0, 255] |
| `0xA` | `pix x, y` | Draw a pixel at ($x, $y), ignore out of bounds |


## Loadtime Errors

- **Invalid Signature**: If the program signature does not match 'g2', terminate with an error.
- **Not Enough Memory**: If the program specifies a memory size less than `32`, terminate with an error.
- **Data Size Error**: If the span of data values exceeds memory bounds, terminate with an error.


## Runtime Errors

- **Zero Division**: If the denominator of a `div` or `mod` instruction is `0`, terminate with an error.
- **Out of Bounds Access**: If any instruction or argument attempts to read an address less than `0` or greater than the amount of allocated memory, terminate with an error.
- **Out of Bounds Instruction**: If the instruction index of a `jne` instruction is less than `0` or greater than the instruction count, terminate with an error.


## Binary Format

g2 binary files have the extension `.g2b`. All integers are stored in little endian byte order. The files are formatted according to the `Program` struct in the following C code (**without padding**):

```c
struct Program {
    Header header;
    uint32_t instruction_count;        // The number of instructions in the program
    Instruction instructions[]
    uint32_t data_start_address;       // The address in memory to start loading data to
    uint32_t data_length;              // The number of data values
    int32_t data_values[data_length];  // The data values
}

// Size: 14 bytes
struct Header {
    uint16_t signature;   // "g2"
    uint32_t memory_size;  // The amount of memory for the program
    uint32_t width;        // The width of the program window
    uint32_t height;       // The height of the program window
}

// Size: 13 bytes
struct Instruction {
    uint8_t opcode;
    int32_t args[3];      // Not all instructions use 3 slots here
}
```

> **Note**: This format is documented in C structs for illustrative purposes only. The actual implementation need not use this exact code.


## Miscellaneous Details

- The current color should be preserved across frames.
- Writing to reserved memory slots is technically allowed, but is discouraged since most of the values will be overwritten at the start of the next frame.
- When using `add` and `mul` instructions, integers should be allowed to overflow and underflow.
