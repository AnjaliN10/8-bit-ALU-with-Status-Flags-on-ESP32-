/*
 * 8-bit ALU with Status Flags on ESP32 (Simulation Mode)
 * ------------------------------------------------------
 * Single-file C implementation of a simple 8-bit ALU.
 * Designed to be hardware-agnostic: no GPIO, no ESP-IDF headers.
 * You can compile it with any standard C compiler or treat it
 * as pseudocode for an ESP32-compatible ALU implementation.
 */

#include <stdio.h>
#include <stdint.h>

/* -------------------- ALU Flags -------------------- */

typedef struct {
    uint8_t Z;  // Zero
    uint8_t C;  // Carry
    uint8_t N;  // Negative (MSB)
    uint8_t V;  // Overflow (2's complement)
} ALUFlags;

/* -------------------- ALU Operations -------------------- */

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_SHL,   // Shift left logical
    OP_SHR    // Shift right logical
} ALUOp;

/* Utility to clear all flags */
static void clear_flags(ALUFlags *f) {
    f->Z = f->C = f->N = f->V = 0;
}

/*
 * Compute overflow for ADD (2's complement)
 */
static uint8_t overflow_add(uint8_t a, uint8_t b, uint8_t r) {
    // Overflow occurs if sign of a == sign of b and sign of result != sign of a
    return (uint8_t)((~(a ^ b) & (a ^ r) & 0x80) != 0);
}

/*
 * Compute overflow for SUB (a - b) in 2's complement
 */
static uint8_t overflow_sub(uint8_t a, uint8_t b, uint8_t r) {
    // Overflow occurs if sign of a != sign of b and sign of result != sign of a
    return (uint8_t)(((a ^ b) & (a ^ r) & 0x80) != 0);
}

/*
 * Core ALU function
 * -----------------
 * a, b  : 8-bit operands
 * op    : operation selector
 * out   : pointer to result
 * flags : pointer to flags structure
 */
void alu_execute(uint8_t a, uint8_t b, ALUOp op, uint8_t *out, ALUFlags *flags) {
    uint16_t temp = 0;  // 16-bit for carry detection

    clear_flags(flags);

    switch (op) {
        case OP_ADD:
            temp = (uint16_t)a + (uint16_t)b;
            *out = (uint8_t)(temp & 0xFF);
            flags->C = (uint8_t)(temp > 0xFF);         // carry out
            flags->V = overflow_add(a, b, *out);       // signed overflow
            break;

        case OP_SUB:
            temp = (uint16_t)a - (uint16_t)b;
            *out = (uint8_t)(temp & 0xFF);
            // Here we set Carry = 1 when a < b (borrow)
            flags->C = (uint8_t)(a < b);
            flags->V = overflow_sub(a, b, *out);
            break;

        case OP_AND:
            *out = (uint8_t)(a & b);
            break;

        case OP_OR:
            *out = (uint8_t)(a | b);
            break;

        case OP_XOR:
            *out = (uint8_t)(a ^ b);
            break;

        case OP_SHL:
            flags->C = (uint8_t)((a & 0x80) != 0);   // MSB before shift
            *out = (uint8_t)(a << 1);
            break;

        case OP_SHR:
            flags->C = (uint8_t)((a & 0x01) != 0);   // LSB before shift
            *out = (uint8_t)(a >> 1);
            break;

        default:
            *out = 0;
            break;
    }

    // Common flag updates
    flags->Z = (uint8_t)(*out == 0);
    flags->N = (uint8_t)((*out & 0x80) != 0);
}

/* -------------------- Pretty Printing Helpers -------------------- */

const char* op_to_string(ALUOp op) {
    switch (op) {
        case OP_ADD: return "ADD";
        case OP_SUB: return "SUB";
        case OP_AND: return "AND";
        case OP_OR:  return "OR";
        case OP_XOR: return "XOR";
        case OP_SHL: return "SHL";
        case OP_SHR: return "SHR";
        default:     return "UNK";
    }
}

/* Print result and flags in a compact way */
void print_result(uint8_t a, uint8_t b, ALUOp op,
                  uint8_t result, const ALUFlags *flags) {

    if (op == OP_SHL || op == OP_SHR) {
        printf("%s  0x%02X -> 0x%02X  | Z=%d C=%d N=%d V=%d\n",
               op_to_string(op),
               a,
               result,
               flags->Z, flags->C, flags->N, flags->V);
    } else {
        printf("%s  0x%02X , 0x%02X -> 0x%02X  | Z=%d C=%d N=%d V=%d\n",
               op_to_string(op),
               a, b,
               result,
               flags->Z, flags->C, flags->N, flags->V);
    }
}

/* -------------------- Simple Test Harness -------------------- */

typedef struct {
    uint8_t a;
    uint8_t b;
    ALUOp   op;
} TestVector;

int main(void) {
    printf("8-bit ALU with Status Flags (Simulation Mode)\n");
    printf("------------------------------------------------\n\n");

    // A small set of example test vectors
    TestVector tests[] = {
        { 15,  27, OP_ADD },
        { 10,  40, OP_SUB },
        { 0xF0, 0x0F, OP_AND },
        { 0xF0, 0x0F, OP_OR  },
        { 0x55, 0xFF, OP_XOR },
        { 0x81, 0x00, OP_SHL },
        { 0x03, 0x00, OP_SHR }
    };

    const size_t NUM_TESTS = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < NUM_TESTS; ++i) {
        uint8_t result = 0;
        ALUFlags flags;

        alu_execute(tests[i].a,
                    tests[i].b,
                    tests[i].op,
                    &result,
                    &flags);

        print_result(tests[i].a, tests[i].b, tests[i].op, result, &flags);
    }

    printf("\nEnd of ALU demonstration.\n");
    return 0;
}
