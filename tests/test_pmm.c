#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Mock PMM Environment ---
#define BITMAP_SIZE_BYTES                                                      \
  32 // 32 bytes = 256 bits (representing 256 memory pages)
static uint8_t bitmap[BITMAP_SIZE_BYTES];

// --- Your Functions ---
static void bitmap_set(size_t bit) { bitmap[bit / 8] |= (1 << (bit % 8)); }
static void bitmap_clear(size_t bit) { bitmap[bit / 8] &= ~(1 << (bit % 8)); }
static bool bitmap_test(size_t bit) {
  return (bitmap[bit / 8] & (1 << (bit % 8))) != 0;
}

// --- Test Cases ---

void test_initial_state() {
  printf("Running test_initial_state... ");
  // Ensure the bitmap is completely zeroed out to start
  memset(bitmap, 0, BITMAP_SIZE_BYTES);

  for (size_t i = 0; i < BITMAP_SIZE_BYTES * 8; i++) {
    assert(bitmap_test(i) == false);
  }
  printf("PASSED\n");
}

void test_single_bit_set_and_clear() {
  printf("Running test_single_bit_set_and_clear... ");
  memset(bitmap, 0, BITMAP_SIZE_BYTES);

  // Set bit 5
  bitmap_set(5);
  assert(bitmap_test(5) == true);

  // Ensure bit 4 and 6 are still clear
  assert(bitmap_test(4) == false);
  assert(bitmap_test(6) == false);

  // Clear bit 5
  bitmap_clear(5);
  assert(bitmap_test(5) == false);
  printf("PASSED\n");
}

void test_byte_boundaries() {
  printf("Running test_byte_boundaries... ");
  memset(bitmap, 0, BITMAP_SIZE_BYTES);

  // Set bits on the edge of the first and second byte
  bitmap_set(7); // Last bit of byte 0
  bitmap_set(8); // First bit of byte 1

  assert(bitmap_test(7) == true);
  assert(bitmap_test(8) == true);

  // Check surrounding bits
  assert(bitmap_test(6) == false);
  assert(bitmap_test(9) == false);

  // Verify at the actual array level (byte 0 should be 10000000, byte 1 should
  // be 00000001)
  assert(bitmap[0] == 0b10000000);
  assert(bitmap[1] == 0b00000001);
  printf("PASSED\n");
}

void test_pattern_stress() {
  printf("Running test_pattern_stress... ");
  memset(bitmap, 0, BITMAP_SIZE_BYTES);

  // Set all EVEN bits
  for (size_t i = 0; i < BITMAP_SIZE_BYTES * 8; i += 2) {
    bitmap_set(i);
  }

  // Verify pattern (Even = true, Odd = false)
  for (size_t i = 0; i < BITMAP_SIZE_BYTES * 8; i++) {
    if (i % 2 == 0) {
      assert(bitmap_test(i) == true);
    } else {
      assert(bitmap_test(i) == false);
    }
  }
  printf("PASSED\n");
}

void test_out_of_bounds_protection() {
  // Note: Your current functions DO NOT have out-of-bounds protection.
  // In a real PMM, setting a bit beyond your maximum physical memory will
  // cause memory corruption (buffer overflow).
  //
  // A robust implementation should look like this:
  // static void bitmap_set(size_t bit) {
  //     if (bit >= MAX_BITS) return; // Or trigger a kernel panic
  //     ...
  // }
}

int main() {
  printf("Starting PMM Bitmap Tests...\n\n");

  test_initial_state();
  test_single_bit_set_and_clear();
  test_byte_boundaries();
  test_pattern_stress();

  printf("\nAll tests completed successfully!\n");
  return 0;
}
