#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include <stdio.h>

typedef unsigned char u8;
typedef unsigned int u16;

struct chip8 {
    u8 memory[4 * 1024];
    u16 stack[16];
    u8 display[32][64];

    // registers
    u8 V[16];
    u16 I;

    // 'pseudo'-registers
    u16 PC;             // program counter
    u8 SP;              // stack pointer
    u8 DT;              // delay timer
    u8 ST;              // sound timer
};


void init_c8(struct chip8* c8);


int main() {
    struct chip8 c8;
    init_c8(&c8);

    const int screenWidth = 64*10;
    const int screenHeight = 32*10;

    InitWindow(screenWidth, screenHeight, "CHIP-8");
    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update

        // Draw
        BeginDrawing();

            ClearBackground(RAYWHITE);

            // Render screen
            for (int y = 0; y < 32; y++)
                for (int x = 0; x < 64; x++) 
                    if (c8.display[y][x])
                        DrawRectangle(x*10, y*10, 10, 10, BLACK);

        EndDrawing();
    }

    CloseWindow();
    return EXIT_SUCCESS;
}


void init_c8(struct chip8* c8) {
    // init display
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 64; x++) {
            if (x % 2 == 0 || y % 2 == 0)
                c8->display[y][x] = 1;
            else
                c8->display[y][x] = 0;

        }

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
}