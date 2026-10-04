#include <cstring>

struct S {
  int i;
  int j;
};

/*
 * Compiles to (x86_64):
 * update_data_ub(char*, int):
 *        mov     dword ptr [rdi + 20], esi
 *        ret
 */
void update_data_ub(char *blob, int new_value) {
  /* This is actually undefined behaviour even though, more likely than not,
   * this will produce 'correct' assembly code.
   *
   * Why UB?
   * because we essentially said the following to the compiler:
   *  "see this sequence of bytes? Interpret them as S objects without actually
   * starting the lifetime of said objects."
   */
  reinterpret_cast<S *>(blob)[2].j = new_value;
}

/*
 * Compiles to (x86_64):
 * update_data_not_ub_using_memcpy(char*, int):
 *      mov     dword ptr [rdi + 20], esi
 *      ret
 * So the compiler (with -O3) optimized these copies away.
 * This is ugly and so on but it is not UB.
 */
void update_data_not_ub_using_memcpy(char *blob, int value) {
  S obj[3];
  memcpy(obj, blob, sizeof(obj));
  obj[2].j = value;
  memcpy(blob, obj, sizeof(obj));
}

void update_data_not_ub_using_bitcast(char *blob, int value) {}
