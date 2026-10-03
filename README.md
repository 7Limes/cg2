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


## Binary Format

```c
struct Program {
    Header header;
    Instruction instructions[]
    uint8_t code_segment_end;          // 0xff (Read instructions until a 0xff opcode is found)
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
