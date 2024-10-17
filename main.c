#define DEBUG 1

#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <raylib.h>

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
    u16 PC;             // program counter
    signed char SP;     // stack pointer
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

void print_stack(struct Chip8* c8);

void init_c8(struct Chip8* c8, const char* romname);
void stack_push(struct Chip8* c8, u16 data);
u16 stack_pop(struct Chip8* c8);
// instructions
void clear_screen(struct Chip8* c8);
void draw_sprite(struct Chip8* c8, u8 X, u8 Y, u8 N);


int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("ERROR: NO ROM FILE PASSED AS ARGUMENT OR TOO MANY ARGUMENTS\n");
        exit(EXIT_FAILURE);
    }

    struct Chip8 c8;

    #if DEBUG > 0
        const int screenWidth = 100*10;
        const int screenHeight = 34*10;
    #else
        const int screenWidth = 66*10;
        const int screenHeight = 34*10;
    #endif

    srand(time(NULL));
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
                        debug_print("%s\n", "CLS");
                        clear_screen(&c8);
                        break;
                    
                    // RET
                    case 0xEE:
                        debug_print("%s\n", "RET");
                        c8.PC = stack_pop(&c8);
                        break;

                    default:
                        debug_print("Skipping instruction: %04x\n", instruction);
                        break;
                }
            break;
            
            // JP
            case 1:
                debug_print("JP1 (PC = %x, NNN = %x)\n", c8.PC, nibbles.NNN);
                c8.PC = nibbles.NNN;
                break;
            
            // CALL
            case 2:
                debug_print("%s\n", "CALL");
                stack_push(&c8, c8.PC);
                c8.PC = nibbles.NNN;
                break;
            
            // SE Vx, byte
            case 3:
                debug_print("%s\n", "SE Vx, byte");
                if (c8.V[nibbles.X] == nibbles.NN)
                    c8.PC += 2;
            
            // SNE Vx, byte
            case 4:
                debug_print("%s\n", "SNE Vx, byte");
                if (c8.V[nibbles.X] != nibbles.NN)
                    c8.PC += 2;

            // SE Vx, Vy
            case 5:
                debug_print("%s\n", "SE Vx, Vy");
                if (c8.V[nibbles.X] == c8.V[nibbles.Y])
                    c8.PC += 2;

            // LD Vx, byte
            case 6:
                debug_print("%s\n", "LD V");
                c8.V[nibbles.X] = nibbles.NN;
                break;

            // ADD 
            case 7:
                debug_print("%s\n", "ADD");
                c8.V[nibbles.X] += nibbles.NN;
                break;

            case 8:
                switch (nibbles.N) {
                    case 0:
                        debug_print("%s\n", "LD Vx, Vy");
                        c8.V[nibbles.X] = c8.V[nibbles.Y];
                        break;

                    case 1:
                        debug_print("%s\n", "OR Vx, Vy");
                        c8.V[nibbles.X] |= c8.V[nibbles.Y];
                        break;

                    case 2:
                        debug_print("%s\n", "AND Vx, Vy");
                        c8.V[nibbles.X] &= c8.V[nibbles.Y];
                        break;
 
                    case 3:
                        debug_print("%s\n", "XOR Vx, Vy");
                        c8.V[nibbles.X] ^= c8.V[nibbles.Y];
                        break;

                    case 4:
                        debug_print("%s\n", "ADD Vx, Vy");
                        c8.V[0xF] = 0;
                        u16 result = c8.V[nibbles.X] + c8.V[nibbles.Y];
                        if (result > 255)
                            c8.V[0xF] = 1;
                        c8.V[nibbles.X] = result;
                        break;

                    case 5:
                        debug_print("%s\n", "SUB Vx, Vy");
                        c8.V[0xF] = 0;
                        if (c8.V[nibbles.X] > c8.V[nibbles.Y])
                            c8.V[0xF] = 1;
                        c8.V[nibbles.X] -= c8.V[nibbles.Y];
                        break;

                    case 6:
                        debug_print("%s\n", "SHR Vx, Vy");
                        c8.V[0xF] = 0;
                        if (c8.V[nibbles.X] & 1)
                            c8.V[0xF] = 1;
                        c8.V[nibbles.X] /= 2;
                        break;

                    case 7:
                        debug_print("%s\n", "SUBN Vx, Vy");
                        c8.V[0xF] = 0;
                        if (c8.V[nibbles.Y] > c8.V[nibbles.X])
                            c8.V[0xF] = 1;
                        c8.V[nibbles.X] = c8.V[nibbles.Y] - c8.V[nibbles.X];
                        break;

                    case 0xE:
                        debug_print("%s\n", "SHL Vx, Vy");
                        c8.V[0xF] = 0;
                        if (c8.V[nibbles.X] & (1 << 7))
                            c8.V[0xF] = 1;
                        c8.V[nibbles.X] *= 2;
                        break;                    
 
                    default:
                        debug_print("This instruction shouldn't exist: %04x\n", instruction);
                        break;
                }

                break;
            // NEED FIXING
            case 9:
                debug_print("%s\n", "SNE Vx, Vy");
                if (c8.V[nibbles.X] != c8.V[nibbles.Y])
                    c8.PC += 2;
                break;

            // LD I, addr
            case 0xA:
                debug_print("%s\n", "LD I");
                c8.I = nibbles.NNN;
                break;

            case 0xB:
                debug_print("%s\n", "JP V0, addr");
                c8.PC = nibbles.NNN + c8.V[0];
                break;

            case 0xC:
                debug_print("%s\n", "RND Vx, byte");
                u8 randomValue = rand() % (255 + 1);
                c8.V[nibbles.X] = randomValue & nibbles.NN;
                break;

            // DRW Vx, Vy, nibble
            case 0xD:
                debug_print("%s\n", "DRW");
                draw_sprite(&c8, c8.V[nibbles.X], c8.V[nibbles.Y], nibbles.N);
                break;

            case 0xE:
                switch (nibbles.NN) {
                    // SKP Vx
                    case 0x9E:
                        debug_print("%s\n", "SKP Vx Not implemented(keyboard)");
                        break;
                    // SKNP Vx
                    case 0xA1:
                        debug_print("%s\n", "SKNP Vx Not implemented(keyboard)");
                        break;
                }
                break;            

            case 0xF:
                switch (nibbles.NN) {
                    // LD Vx, DT
                    case 0x07:
                        debug_print("%s\n", "LD Vx, DT");
                        c8.V[nibbles.X] = c8.DT;
                        break;
                    // LD Vx, K
                    case 0x0A:
                        debug_print("%s\n", "LD Vx, K Not implemented(keyboard)");
                        break;
                    // LD DT, Vx
                    case 0x15:
                        debug_print("%s\n", "LD DT, Vx");
                        c8.DT = c8.V[nibbles.X];
                        break;
                    // LD ST, Vx
                    case 0x18:
                        debug_print("%s\n", "LD ST, Vx");
                        c8.ST = c8.V[nibbles.X];
                        break;
                    // ADD I, Vx
                    case 0x1E:
                        debug_print("%s\n", "ADD I, Vx");
                        c8.I += c8.V[nibbles.X];
                        break;
                    // LD F, Vx
                    case 0x29:
                        debug_print("%s\n", "LD F, Vx");
                        c8.I = (nibbles.X * 5) + 0x050;
                        break;

                    // NEEDS FIXING
                    // LD B, Vx
                    case 0x33:
                        debug_print("%s\n", "LD B, Vx");
                        c8.memory[c8.I] = nibbles.X / 100;
                        c8.memory[c8.I+1] = (nibbles.X % 100) / 10;
                        c8.memory[c8.I+2] = (nibbles.X % 10);
                        break;
                    // LD [I], Vx
                    case 0x55:
                        debug_print("%s\n", "LD [I], Vx");
                        for (size_t i = 0; i <= nibbles.X; i++) {
                            c8.memory[c8.I+i] = c8.V[i];
                        }
                        break;
                    // LD Vx, [I]
                    case 0x65:
                        debug_print("%s\n", "LD Vx, [I]");
                        for (size_t i = 0; i <= nibbles.X; i++) {
                            c8.V[i] = c8.memory[c8.I+i];
                        }
                        break;
                }
                break;

            default:
                debug_print("UNKNOWN INSTRUCTION: %04x\n", instruction);
                break;
        }

        /* Draw */
        BeginDrawing();

            ClearBackground(BLACK);

            // Draw border
            DrawRectangleLines(10, 10, 64*10, 32*10, GREEN);

            // Print debug information
            #if DEBUG > 0
                int y = 0;
                DrawText("Registers:", 64*10+10+10, y+=10, 16, WHITE);
                for (size_t i = 0; i < 16; i++)
                    DrawText(TextFormat("V[%d] = %d", i, c8.V[i]), 64*10+10+10, y+=18, 16, WHITE);

                y = 0;
                DrawText(TextFormat("I = 0x%04x", c8.I), 64*10+10+10+120, y+=10, 16, WHITE);
                DrawText(TextFormat("PC = 0x%04x", c8.PC), 64*10+10+10+120, y+=18, 16, WHITE);
                DrawText(TextFormat("SP = %d", c8.SP), 64*10+10+10+120, y+=18, 16, WHITE);
                DrawText("Timers:", 64*10+10+10+120, y+=18, 16, WHITE);
                DrawText(TextFormat("DT = %d", c8.DT), 64*10+10+10+120, y+=18, 16, WHITE);
                DrawText(TextFormat("ST = %d", c8.ST), 64*10+10+10+120, y+=18, 16, WHITE);
                DrawText("Stack:", 64*10+10+10+120, y+=18, 16, WHITE);
                if (c8.SP > -1) {
                    for (size_t i = 0; i <= c8.SP; i++)
                        DrawText(TextFormat("S[i] = 0x%04x", c8.stack[i]), 64*10+10+10+120, y+=18, 16, WHITE);
                }
            #endif

            // Render screen
            for (size_t y = 0; y < 32; y++)
                for (size_t x = 0; x < 64; x++) 
                    if (c8.display[y][x])
                        DrawRectangle((x+1)*10, (y+1)*10, 10, 10, WHITE);

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
    c8->SP = -1;
    for (size_t i = 0; i < 12; i++)
        c8->stack[i] = 0;
}

void stack_push(struct Chip8* c8, u16 data) {
    c8->stack[++c8->SP] = data;
    if (c8->SP > 11) {
        printf("ERROR: STACK OVERFLOW\n");
        exit(EXIT_FAILURE);
    }
}

u16 stack_pop(struct Chip8* c8) {
    u16 temp = c8->stack[c8->SP];
    c8->stack[c8->SP--] = 0;
    if (c8->SP < -1) {
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

void draw_sprite(struct Chip8 *c8, u8 X, u8 Y, u8 N) {
    u8 x = X % 63;
    u8 y = Y % 31;
    c8->V[0xF] = 0;
    
    for (int i = 0; i < N; i++) {
        if (y + i >= 32)
            continue;
        
        u8 current_byte = c8->memory[(c8->I)+i];
        for (int j = 7, x_pos = 0; j >= 0; j--, x_pos++) {
            if (x + x_pos >= 64)
                continue;
            if (c8->display[y+i][x+x_pos] && (current_byte >> j) & 1)
                c8->V[0xF] = 1;
            c8->display[y+i][x+x_pos] ^= (current_byte >> j) & 1;
        }
    }
}

void print_stack(struct Chip8 *c8) {
    if (c8->SP < 0) {
        printf("empty stack\n");
        return;
    }
    printf("SP: %d, stack: ", c8->SP);
    for (size_t i = 0; i <= c8->SP; i++)
        printf("%04x ", c8->stack[i]);
    printf("\n");
}