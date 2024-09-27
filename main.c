#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include <stdio.h>

typedef unsigned char u8;
typedef unsigned int u16;

struct Chip8 {
    u8 memory[4 * 1024];
    u16 stack[16];
    u8 display[32][64];

    // registers
    u8 V[16];
    u16 I;

    // 'pseudo'-registers
    u16 PC;            // program counter
    u8 SP;              // stack pointer
    u8 DT;              // delay timer
    u8 ST;              // sound timer
};

struct Nibbles {
    u8 type;
    u8 X;
    u8 Y;
    u8 N;
    u8 NN;
    u16 NNN;
};


void init_c8(struct Chip8* c8, const char* romname);
void stack_push(struct Chip8* c8, u16 data);
u16 stack_pop(struct Chip8* c8);
// instructions
void clear_screen(struct Chip8* c8);
void draw_sprite(struct Chip8 *c8, u8 X, u8 Y, u8 N);


int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("ERROR: NO ROM FILE PASSED AS ARGUMENT OR TOO MANY ARGUMENTS\n");
        exit(EXIT_FAILURE);
    }

    struct Chip8 c8;
    const int screenWidth = 64*10;
    const int screenHeight = 32*10;

    init_c8(&c8, argv[1]);
    InitWindow(screenWidth, screenHeight, "CHIP-8");
    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose())
    {
        /* Update */
        // Decrement timers (im not sure if update is tied to fps yet, hope it is)
        if (c8.DT)  c8.DT--;
        if (c8.ST)  c8.SP--;
        
        // FETCH
        u16 instruction = (c8.memory[c8.PC++] << 8) | c8.memory[c8.PC++];
        
        // DECODE
        struct Nibbles nibbles = {
            .type = (instruction >> 12) & 0b1111,
            .X = (instruction >> 8) & 0b1111,
            .Y = (instruction >> 4) & 0b1111,
            .N = instruction & 0b1111,
            .NN = instruction,
            .NNN = instruction & 0xFFF,
        };

        // EXECUTE
        switch (nibbles.type) {
            case 0: 
                switch (nibbles.NN) {
                    // CLS
                    case 0xE0:  
                        clear_screen(&c8);
                        break;
                    
                    // RET
                    case 0xEE:
                        c8.PC = stack_pop(&c8);
                        break;

                    default:
                        break;
                }
            
            // JP
            case 1:
                c8.PC = nibbles.NNN;
                break;
            
            // CALL
            case 2:
                stack_push(&c8, c8.PC);
                c8.PC = nibbles.NNN;
                break;
            
            // LD Vx, byte
            case 6:
                c8.V[nibbles.X] = nibbles.NN;
                break;

            // ADD
            case 7:
                c8.V[nibbles.X] += nibbles.NN;
                break;

            // LD I, addr
            case 0xA:
                c8.I = nibbles.NNN;
                break;

            // DRW Vx, Vy, nibble
            case 0xD:
                draw_sprite(&c8, c8.V[nibbles.X], c8.V[nibbles.Y], nibbles.N);
                break;

            default:
                break;
        }

        /* Draw */
        BeginDrawing();

            ClearBackground(RAYWHITE);

            // Render screen
            for (size_t y = 0; y < 32; y++)
                for (size_t x = 0; x < 64; x++) 
                    if (c8.display[y][x])
                        DrawRectangle(x*10, y*10, 10, 10, BLACK);

        EndDrawing();
    }

    CloseWindow();
    return EXIT_SUCCESS;
}


void init_c8(struct Chip8* c8, const char* romname) {
    // init display
    clear_screen(c8);
    // fonts
    u8 fonts[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };
    memmove(&(c8->memory[0x050]), fonts, sizeof(fonts));

    // timers
    c8->DT = 0;
    c8->ST = 0;

    // read ROM into memory
    FILE *fp = fopen(romname, "rb");
    if (fp == NULL) {
        printf("ERROR: COULD NOT READ ROM\n");
        exit(EXIT_FAILURE);
    }
    fseek(fp, 0, SEEK_END);
    long int file_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fread(&(c8->memory[0x200]), sizeof(u8), file_len, fp);
    fclose(fp);
    c8->PC = 0x200;

    // stack
    c8->SP = 0;
    for (size_t i = 0; i < 12; i++)
        c8->stack[i] = 0;
}

void stack_push(struct Chip8* c8, u16 data) {
    c8->stack[c8->SP++] = data;
    if (c8->SP > 11) {
        printf("ERROR: STACK OVERFLOW\n");
        exit(EXIT_FAILURE);
    }
}

u16 stack_pop(struct Chip8* c8) {
    u16 temp = c8->stack[c8->SP];
    c8->stack[c8->SP--] = 0;
    if (c8->SP < 0) {
        printf("ERROR: RETURN FROM EMPTY STACK\n");
        exit(EXIT_FAILURE);
    }
    return temp;
}

void clear_screen(struct Chip8* c8) {
    for (size_t y = 0; y < 32; y++)
        for (size_t x = 0; x < 64; x++)
            c8->display[y][x] = 0;
}

// unfinished
void draw_sprite(struct Chip8 *c8, u8 X, u8 Y, u8 N) {
    u8 x = X % 64;
    u8 y = Y % 32;

    for (size_t i = 0; i < N; i += 2) {
        u8 sprite = c8->memory[c8->I + i];
        for (size_t j = 0, k = 3; j < 4; j++, k--) {
            c8->display[y][x+j] ^= (sprite >> k) & 1;
        }
    }
    

}