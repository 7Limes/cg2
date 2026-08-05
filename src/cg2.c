// cg2 Interpreter
// By Miles Burkart (7Limes)
//  

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <SDL2/SDL.h>

const char *CG2_VERSION = "0.1.0";

#define G2_DEBUG

#define FILE_SIGNATURE_LENGTH 2
#define END_CODE_SEGMENT_OPCODE 0xff

#define INSTRUCTION_SIZE 13

#define FRAMERATE 60

#define MIN_MEMORY 64

#define CALL_STACK_POINTER_ADDRESS 15
#define CALL_STACK_ADDRESS 16
#define CALL_STACK_SIZE 16


#define FLAG_BUFFER_SIZE 128
#define FLAG_TITLE_BUFFER_SIZE 64

typedef struct {
    bool show_fps;
    uint32_t scale;
    char title[FLAG_TITLE_BUFFER_SIZE];
} FlagData;


typedef struct {
    uint8_t opcode;
    int32_t args[3];
} Instruction;


typedef struct {
    FlagData *flags;
    uint32_t width, height;

    size_t instruction_count;
    Instruction *instructions;

    uint32_t memory_size;
    int32_t *memory;

    SDL_Window *win;
    SDL_Renderer *renderer;
} ProgramContext;


bool safecat(char* dest, char* src, int size) {
    if (strlen(dest) + strlen(src) >= size)
        return false;
    strncat(dest, src, size-1);
    return true;
}


int32_t read_le_i32(const uint8_t *p) {
    return (int32_t)((uint32_t)p[0]        |
                      (uint32_t)p[1] << 8  |
                      (uint32_t)p[2] << 16 |
                      (uint32_t)p[3] << 24);
}


// Based on https://stackoverflow.com/a/3464656
// Reads data from `file_path` into `output_buffer` and stores the length in `length`.
int read_file_bytes(uint8_t **output_buffer, size_t *length, const char *file_path) {
    if (!output_buffer || !length || !file_path) {
        return -1;  // Invalid arguments
    }

    FILE *handler = fopen(file_path, "rb");
    if (!handler) {
        return -2;  // File cannot be opened
    }
    
    if (fseek(handler, 0, SEEK_END) != 0) {
        fclose(handler);
        return -3;  // Seek error
    }

    long file_size = ftell(handler);
    if (file_size < 0) {
        fclose(handler);
        return -4;  // Tell error
    }
    rewind(handler);

    uint8_t *buffer = malloc((size_t)file_size + 1);
    if (!buffer) {
        fclose(handler);
        return -5;  // Memory allocation failure
    }

    size_t read_size = fread(buffer, sizeof (uint8_t), (size_t) file_size, handler);
    if (read_size != (size_t) file_size) {
        free(buffer);
        fclose(handler);
        return -6;  // Read error
    }

    buffer[file_size] = '\0';
    fclose(handler);

    *output_buffer = buffer;
    *length = (size_t) file_size;

    return 0;  // Success
}


bool file_exists(char* path) {
    struct stat buffer;
    return stat(path, &buffer) == 0;
}


static inline void error(char *message) {
    fprintf(stderr, "\x1b[31mRUNTIME ERROR: %s\n", message);
}


static inline void out_of_bounds_error(int32_t address) {
    char err_buff[256];
    snprintf(err_buff, 256, "Tried to access out of bounds memory at address %d\n", address);
    error(err_buff);
}


size_t get_instruction_count(uint8_t *code_segment_bytes) {
    size_t count = 0;
    for (uint8_t *byte_ptr = code_segment_bytes; *byte_ptr != END_CODE_SEGMENT_OPCODE; byte_ptr+=INSTRUCTION_SIZE) {
        count++;
    }
    return count;
}


void load_program(ProgramContext *context, uint8_t *program_bytes) {
    uint8_t *byte_ptr = program_bytes;

    byte_ptr += FILE_SIGNATURE_LENGTH;  // Skip signature

    // Read metadata
    context->memory_size = (uint32_t) read_le_i32(byte_ptr);
    context->width = (uint32_t) read_le_i32(byte_ptr+4);
    context->height = (uint32_t) read_le_i32(byte_ptr+8);
    byte_ptr += 12;

    // Allocate memory
    context->memory = calloc(context->memory_size, sizeof(int32_t));
    context->memory[CALL_STACK_POINTER_ADDRESS] = CALL_STACK_ADDRESS;

    // Load instructions
    context->instruction_count = get_instruction_count(byte_ptr);
    context->instructions = calloc(context->instruction_count, sizeof(Instruction));

    Instruction *instruction_ptr = context->instructions;

    for (size_t i = 0; i < context->instruction_count; i++) {
        instruction_ptr->opcode = *byte_ptr;
        instruction_ptr->args[0] = read_le_i32(byte_ptr+1);
        instruction_ptr->args[1] = read_le_i32(byte_ptr+5);
        instruction_ptr->args[2] = read_le_i32(byte_ptr+9);

        instruction_ptr++;
        byte_ptr += INSTRUCTION_SIZE;
    }

    // Load data entries
    byte_ptr++;
    uint32_t data_entry_count = (uint32_t) read_le_i32(byte_ptr);
    uint32_t data_entry_address = (uint32_t) read_le_i32(byte_ptr+4);
    byte_ptr += 8;

    int32_t *memory_ptr = &context->memory[data_entry_address];
    for (uint32_t i = 0; i < data_entry_count; i++) {
        uint32_t entry_size = (uint32_t) read_le_i32(byte_ptr);
        byte_ptr += 4;
        for (uint32_t j = 0; j < entry_size; j++) {
            *memory_ptr = read_le_i32(byte_ptr);
            byte_ptr += 4;
            memory_ptr++;
        }
    }
}


void quit_sdl(ProgramContext *context) {
    SDL_Window *win = context->win;
    SDL_Renderer *renderer = context->renderer;

    if (win) {
        SDL_DestroyWindow(win);
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }

    SDL_Quit();
}


int init_sdl(ProgramContext *context, uint32_t width, uint32_t height, const char *title) {
    // Initialize
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Failed to initialize SDL");
        return 1;
    }

    // Create window
    context->win = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
    if (!context->win) {
        fprintf(stderr, "Failed to create window");
        return 2;
    }

    // Create renderer
    context->renderer = SDL_CreateRenderer(context->win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!context->renderer) {
        fprintf(stderr, "Failed to create SDL renderer");
        quit_sdl(context);
        return 3;
    }

    // Scale renderer
    int set_scale_response = SDL_RenderSetScale(context->renderer, context->flags->scale, context->flags->scale);
    if (set_scale_response < 0) {
        fprintf(stderr, "Failed to set renderer scale");
        quit_sdl(context);
        return 4;
    }

    return 0;
}

static inline int32_t get_mem(const ProgramContext *context, int *error, int32_t address) {
    #ifdef G2_DEBUG
        if (address < 0 || address >= context->memory_size) {
            out_of_bounds_error(address);
            *error = 1;
            return -1;
        }
    #endif
    return context->memory[address];
}

static inline int32_t set_mem(const ProgramContext *context, int32_t dest, int32_t value) {
    #ifdef G2_DEBUG
        if (dest < 0 || dest >= context->memory_size) {
            out_of_bounds_error(dest);
            return 1;
        }
    #endif

    context->memory[dest] = value;
    return 0;
}


static inline int execute_program(ProgramContext *context) {
    static void *dispatch_table[] = {
        &&do_ldi, &&do_rdi, &&do_sti,
        &&do_add, &&do_mul, &&do_div, &&do_mod,
        &&do_cmp, &&do_jne, &&do_col, &&do_pix
    };

    int ins_error = 0;

    int32_t a, b, r, g, x, y;
    uint32_t program_counter = 0;
    
    dispatch:
        #ifdef G2_DEBUG
        if (context->memory[CALL_STACK_POINTER_ADDRESS] > CALL_STACK_ADDRESS + CALL_STACK_SIZE) {
            char err_buf[128];
            snprintf(err_buf, 127, "Stack overflow at index %d", program_counter);
            error(err_buf);
            ins_error = 1;
        }
        #endif
        if (ins_error) {
            return ins_error;
        }

        if (program_counter >= context->instruction_count) {
            return 0;
        }
        
        Instruction ins = context->instructions[program_counter];
        program_counter++;

        int32_t *args = ins.args;

        goto *dispatch_table[ins.opcode];
    
    do_ldi:
        ins_error |= set_mem(context, args[0], args[1]);
        goto dispatch;
    do_rdi:
        int32_t addr = get_mem(context, &ins_error, args[1]);
        int32_t value = get_mem(context, &ins_error, addr);
        ins_error |= set_mem(context, args[0], value);
        goto dispatch;
    do_sti:
        int32_t dest = get_mem(context, &ins_error, args[0]);
        ins_error |= set_mem(context, dest, get_mem(context, &ins_error, args[1]));
        goto dispatch;
    do_add:
        ins_error |= set_mem(context, args[0], get_mem(context, &ins_error, args[1]) + get_mem(context, &ins_error, args[2]));
        goto dispatch;
    do_mul:
        ins_error |= set_mem(context, args[0], get_mem(context, &ins_error, args[1]) * get_mem(context, &ins_error, args[2]));
        goto dispatch;
    do_div:
        ins_error |= set_mem(context, args[0], get_mem(context, &ins_error, args[1]) / get_mem(context, &ins_error, args[2]));
        goto dispatch;
    do_mod:
        a = get_mem(context, &ins_error, args[1]);
        b = get_mem(context, &ins_error, args[2]);
        int32_t mod = a % b;
        if (mod != 0 && (mod < 0) ^ (b < 0)) {
            mod += b;
        }
        ins_error |= set_mem(context, args[0], mod);
        goto dispatch;
    do_cmp:
        a = get_mem(context, &ins_error, args[1]);
        b = get_mem(context, &ins_error, args[2]);
        set_mem(context, args[0], (a < b) - (a > b));
        goto dispatch;
    do_jne:
        if (get_mem(context, &ins_error, args[1]) != get_mem(context, &ins_error, args[2])) {
            program_counter = get_mem(context, &ins_error, args[0]);
        }
        goto dispatch;
    do_col:
        r = get_mem(context, &ins_error, args[0]);
        g = get_mem(context, &ins_error, args[1]);
        b = get_mem(context, &ins_error, args[2]);
        SDL_SetRenderDrawColor(context->renderer, r, g, b, SDL_ALPHA_OPAQUE);
        goto dispatch;
    do_pix:
        x = get_mem(context, &ins_error, args[0]);
        y = get_mem(context, &ins_error, args[1]);
        SDL_RenderDrawPoint(context->renderer, x, y);
        goto dispatch;

    return 0;
}


static inline void update_reserved_memory(const ProgramContext *context, const Uint8 *keys, uint64_t delta_ms) {
    int32_t values[] = {
        0,
        1,
        -1,
        keys[SDL_SCANCODE_RETURN],
        keys[SDL_SCANCODE_RSHIFT],
        keys[SDL_SCANCODE_Z],
        keys[SDL_SCANCODE_X],
        keys[SDL_SCANCODE_UP],
        keys[SDL_SCANCODE_DOWN],
        keys[SDL_SCANCODE_LEFT],
        keys[SDL_SCANCODE_RIGHT],
        delta_ms
    };

    memcpy(context->memory, values, sizeof(values));
}


int program_loop(ProgramContext *context) {
    Uint32 target_frame_time = 1000 / FRAMERATE;
    Uint64 last_frame_time = 0, start_frame_time = 0;
    int32_t delta_ms = 0;

    SDL_Texture *canvas = SDL_CreateTexture(
        context->renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
        context->width, context->height
    );

    const Uint8 *keyboard = SDL_GetKeyboardState(NULL);

    bool running = true; 
    while (running) {
        start_frame_time = SDL_GetTicks64();
        delta_ms = start_frame_time - last_frame_time;
        last_frame_time = start_frame_time;

        SDL_Event e;
        while (SDL_PollEvent(&e) > 0) {
            switch (e.type) {
                case SDL_QUIT:
                    running = false;
                    break;
            }
        }
        SDL_PumpEvents();

        SDL_SetRenderTarget(context->renderer, canvas);
        
        update_reserved_memory(context, keyboard, delta_ms);
        int execute_result = execute_program(context);
        if (execute_result) {
            running = false;
        }

        SDL_SetRenderTarget(context->renderer, NULL);
        SDL_RenderCopy(context->renderer, canvas, NULL, NULL);
        SDL_RenderPresent(context->renderer);

        uint64_t frame_time = SDL_GetTicks64() - start_frame_time;
        if (frame_time < target_frame_time) {
            SDL_Delay(target_frame_time - frame_time);
        }
    }
}


void parse_flags(FlagData* flags, int argc, char **argv) {
    flags->show_fps = false;
    flags->scale = 1;
    flags->title[0] = '\0';

    if (argc == 2) {  // No flags provided
        return;
    }

    for (int i = 2; i < argc; i++) {
        char *flag = argv[i];
        bool is_last = i == argc-1;

        // FPS flag
        if (strcmp(flag, "--show_fps") == 0 || strcmp(flag, "-fps") == 0) {
            flags->show_fps = true;
        }

        // Scale flag
        else if (strcmp(flag, "--scale") == 0 || strcmp(flag, "-s") == 0) {
            if (is_last) {
                fprintf(stderr, "Expected a value for pixel size flag.\n");
                continue;
            }
            flag = argv[++i];
            uint32_t possible_pixel_size = atoi(flag);
            if (possible_pixel_size == 0) {
                fprintf(stderr, "Expected numeric or nonzero value for pixel size flag.\n");
                continue;
            }
            flags->scale = possible_pixel_size;
        }

        // Title flag
        else if (strcmp(flag, "--title") == 0 || strcmp(flag, "-t") == 0) {
            if (is_last) {
                fprintf(stderr, "Expected a string value for title flag.\n");
                continue;
            }
            flag = argv[++i];
            // Replace underscores with spaces
            for (char *s = flag; s[0] != '\0'; s++) {
                if (*s == '_') {
                    *s = ' ';
                }
            }
            safecat(flags->title, flag, FLAG_TITLE_BUFFER_SIZE);
        }

        // Unrecognized flag
        else {
            fprintf(stderr, "Unrecognized flag \"%s\".\n", flag);
        }
    }
}


int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("usage: cg2 program_path [--show_fps] [--scale SCALE] [--title TITLE]\n");
        return 1;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("cg2 Interpreter %s\n", CG2_VERSION);
        return 0;
    }
    
    if (!file_exists(argv[1])) {
        fprintf(stderr, "File \"%s\" does not exist.\n", argv[1]);
        return 3;
    }

    FlagData flags = {0};
    parse_flags(&flags, argc, argv);
    
    ProgramContext context = {0};
    context.flags = &flags;
    
    uint8_t *program_bytes;
    size_t program_bytes_length;
    read_file_bytes(&program_bytes, &program_bytes_length, argv[1]);
    
    load_program(&context, program_bytes);
    free(program_bytes);

    char *title = flags.title[0] == '\0' ? "cg2" : flags.title;
    init_sdl(&context, context.width*flags.scale, context.height*flags.scale, title);
    
    program_loop(&context);

    quit_sdl(&context);
    free(context.instructions);
    free(context.memory);
}