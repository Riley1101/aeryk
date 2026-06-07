#include <gdt.h>

void gdt_flush(struct gdt_ptr_struct *ptr) { (void)ptr; }
void tss_flush(void) {}
