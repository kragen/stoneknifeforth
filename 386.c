/* Emulate enough of a 386 on Linux to run StoneKnifeForth.
 *
 * Doesn’t quite work yet.
 *
 * StoneKnifeForth generates i386 machine code in an ELF executable
 * file, which would seem to imply that you need an i386 or compatible
 * to run the resulting executable.  That *is* the fastest way to do
 * it, but it’s by no means the only way.  The subset of 386 machine
 * code used by StoneKnifeForth is small enough to implement easily as
 * an interpreter.
 *
 * The system calls that are used at the moment are _exit (#1), read
 * (#3), and write (#4).
 */
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t s32;

typedef struct {
  size_t brk;
  u8 zf, sf, tracing_stacks, tracing_eip, *ram;
  u32 eax, ebx, ecx, edx, esp, ebp, eip;
} terp_t;

static void
ramdump(terp_t *terp, u32 start, u32 len)
{
  for (u32 addr = start; addr - start < len; addr += 16) {
    printf("%08x: ", addr);
    for (int j = 0; j < 16; j += 2) {
      for (int k = 0; k < 2; k++) {
        u32 addr_x = addr + j + k;
        if (addr_x - start >= len) continue;
        if (addr_x < 4096 || addr_x > terp->brk) {
          printf("--");
        } else {
          printf("%02x", terp->ram[addr_x]);
        }
      }
      printf(" ");
    }
    printf("\n");
  }
}

static void
regdump(terp_t *terp)
{
  printf("eip=0x%x, esp=0x%x, ebp=0x%x, brk=0x%x\n",
         terp->eip, terp->esp, terp->ebp, (u32)terp->brk);
  printf("eax=0x%x, ebx=0x%x, ecx=0x%x, edx=0x%x\n",
         terp->eax, terp->ebx, terp->ecx, terp->edx);
  printf("around eip:\n"); ramdump(terp, terp->eip-16, 32);
  printf("around esp:\n"); ramdump(terp, terp->esp-16, 32);
  printf("around ebp:\n"); ramdump(terp, terp->ebp-16, 32);
}

static void
die(char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  if (errno) {
    perror("‌");
  } else {
    fprintf(stderr, "\n");
  }
  va_end(args);
  abort();
}

static inline u32
u32_in(u8 *p)
{
  return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}

static inline void
u32_out(u8 *p, u32 v)
{
  p[0] = v;
  p[1] = v >> 8;
  p[2] = v >> 16;
  p[3] = v >> 24;
}

#define IF break; case
#define ELSE break; default

/* Initialize a terp_t from an ELF executable in RAM.
 */
void load(u8 *elf_file, size_t length, terp_t *terp)
{
  /*
   * To find where the program entry point is located (the e_entry field
   * of Elf32_Ehdr), we need to read at least a bit of the ELF header,
   * and we also need to locate the program executable accurately in its
   * virtual memory; we need to follow the e_phoff pointer in the
   * Elf32_Ehdr to find the Elf32_Phdr, find p_vaddr to figure out where
   * to load the executable, find p_memsz to figure out how big to make
   * its RAM.  Also, it would be good to check that the ELF file is an
   * executable (e_type == 2) for a 386 (e_machine == 3).  These are at:
   * - e_type: offset 16
   * - e_machine: offset 18
   * - e_entry: offset 24 (0x18)
   * - e_phoff: offset 28 (0x1c)
   * - p_vaddr: offset 8
   * - p_memsz: offset 20 (0x14)
   */
  u8 magic[5] = {0};
  memcpy(magic, elf_file, 4);
  if (0 != memcmp(magic, "\177ELF", 4)) {
    die("Invalid ELF file starting with \"%s\"", magic);
  }

  u32 type_machine = u32_in(&elf_file[16]);
  if (0x30002 != type_machine) {
    die("ELF type/machine 0x%x isn't i386 executable", type_machine);
  }
      
  u32 e_entry = u32_in(&elf_file[24])
    , e_phoff = u32_in(&elf_file[28])
    , p_vaddr = u32_in(&elf_file[e_phoff + 8])
    , p_memsz = u32_in(&elf_file[e_phoff + 20])
    ;

  u8 *ram = malloc(p_memsz); /* XXX not sure if this should be + p_vaddr? */
  if (!ram) die("malloc(%d)", p_memsz);
  memset(ram, '\0', p_memsz);
  if (length > p_memsz - p_vaddr) length = p_memsz - p_vaddr;
  memcpy(ram + p_vaddr, elf_file, length);
  terp->brk = p_memsz;
  terp->ram = ram;

  /* XXX need to set up a real stack somewhere! */
  terp->esp = terp->ebp = terp->brk;
  terp->eax = terp->ebx = terp->ecx = terp->edx = 0;
  terp->zf = terp->sf = terp->tracing_stacks = terp->tracing_eip = 0;
  terp->eip = e_entry;
}

/* Translate an address in the program’s memory space to an address in
 * the interpreter’s memory space for an access of a given length.
 */
static void
*translate(terp_t *terp, u32 address, u32 length)
{
  if (address > terp->brk || address + length > terp->brk) {
    regdump(terp);
    die("address 0x%x with length 0x%x exceeds limit 0x%x",
        address,
        length,
        terp->brk);
  }

  if (address < 4096) {
    regdump(terp);
    die("zero-page address access at 0x%x with length 0x%x (eip=0x%x)",
        address,
        length,
        terp->eip);
  }

  return &terp->ram[address];
}

static u32
implement_syscall(terp_t *terp, u32 eax, u32 ebx, u32 ecx, u32 edx)
{
  void *userptr;
  switch(eax) {
  IF 1: exit(ebx); die("exit failed"); return 0;
  /* XXX these two are not indicating errors properly with -errno */
  IF 3: userptr = translate(terp, ecx, edx); return read(ebx, userptr, edx);
  IF 4: userptr = translate(terp, ecx, edx); return write(ebx, userptr, edx);
  ELSE:
    die("unimplemented system call %d (%x, %x, %x)", eax, ebx, ecx, edx);
    return 0;
  }
}

static void
trace(char *vars, ...)
{
  va_list args;
  va_start(args, vars);
  char *vp = vars;
  printf("{");
  while (*vp) {
    while (*vp && isspace(*vp)) vp++;
    while (*vp && !isspace(*vp)) putchar(*vp++);
    printf(": 0x%x", va_arg(args, int));
    if (*vp) printf(", ");
  }
  printf("}\n");
  va_end(args);
}

static inline void
pop(terp_t *terp, u32 *dest)
{
  *dest = u32_in(translate(terp, terp->esp, 4));
  if (terp->tracing_stacks) trace("esp pop", terp->esp, *dest);
  terp->esp += 4;
}

/* sign extend byte to u32 */
static inline u32
sex(u8 v)
{
  return v & 0x80 ? 0xFfffFfff - v : v;
}

static inline s32
sex_dword(u32 v)
{
  return v & (1 << 31) ? -(s32)(~v + 1) : v;
}

static inline void
set_flags(terp_t *terp, u32 v)
{
  terp->zf = !!v;
  terp->sf = !!(v & 0xf0000000);
}

#define req(x) (_req(x, #x, __FILE__, __LINE__))

static inline void
_req(int truth, char *msg, char *file, int line)
{
  if (!truth) die("%s:%d: req failed: %s", file, line, msg);
}

void
single_step(terp_t *terp)
{
  /* StoneKnifeForth (currently) only uses the following 23
   * instructions, in decimal:
   * 
   * - 15 157 192: setge %al
   * - 15 182 0: movzbl (%eax), %eax
   * - 15 190 192: movsbl %al, %eax
   * - 41 4 36: sub %eax, (%esp)
   * - 80: push %eax
   * - 88: pop %eax
   * - 89: pop %ecx
   * - 90: pop %edx
   * - 91: pop %ebx
   * - 116: jz (short, PC-relative) (1-byte operand)
   * - 117: jnz (short, PC-relative) (1-byte operand)
   * - 129 237: subtract immediate from %ebp (4-byte operand)
   * - 133 192: test %eax, %eax
   * - 135 236: xchg %ebp, %esp
   * - 136 8: mov %cl, (%eax)
   * - 137 229: mov %esp, %ebp
   * - 139 0: mov (%eax), %eax
   * - 143 0: popl (%eax)
   * - 184: load immediate %eax (e.g. mov $12345678, %eax) (4-byte operand)
   * - 195: ret
   * - 205 128: int $0x80
   * - 232: PC-relative call to absolute address (4-byte operand)
   * - 254 200: dec %al
   *
   * Moreover, jz and jnz are always preceded two instructions
   * previously by a test instruction, and setge is always preceded two
   * instructions previously by a sub instruction, so of the flags, only
   * ZF and SF are used, and their state is never saved or restored or
   * transferred across jumps.
   *
   */
  u8 *p = translate(terp, terp->eip, 6);
  if (terp->tracing_eip) trace("eip [eip]", terp->eip, *p);

  switch (*p) {
  IF 15:                        /* 0x0f */
    switch (p[1]) {
    IF 157:                     /* setge %al */
      req(p[2] == 192);
      terp->eax &= ~0xff;
      if (terp->sf) terp->eax |= 1; /* XXX wrong, should be SF == OF */
      terp->eip += 3;
    IF 182:                     /* movzbl (%eax), %eax */
      req(p[2] == 0);
      terp->eax = *(u8*)translate(terp, terp->eax, 1);
      terp->eip += 3;
    IF 190:                     /* movsbl %al, %eax */
      req(p[2] == 192);
      terp->eax = sex(terp->eax);
      terp->eip += 3;
    ELSE:
      die("Unimplemented insn 0x0f 0x%x", p[1]);
    }
  IF 41:
    req(p[1] == 4 && p[2] == 36); /* sub %eax, (%esp) */
    u8 *nosp = translate(terp, terp->esp, 4);
    u32 nos = u32_in(nosp)
      , result = nos - terp->eax;
    set_flags(terp, result);
    u32_out(nosp, result);
    terp->eip += 3;
  IF 80:                        /* push %eax */
    terp->esp -= 4;
    u32_out(translate(terp, terp->esp, 4), terp->eax);
    if (terp->tracing_stacks) trace("esp push", terp->esp, terp->eax);
    terp->eip++;
  IF 88:                        /* pop %eax */
    pop(terp, &terp->eax);
    terp->eip++;
  IF 89:                        /* pop %ecx */
    pop(terp, &terp->ecx);
    terp->eip++;
  IF 90:                        /* pop %edx */
    pop(terp, &terp->edx);
    terp->eip++;
  IF 91:                        /* pop %ebx */
    pop(terp, &terp->ebx);
    terp->eip++;
  IF 116:                       /* jz */
    terp->eip += 2;
    if (terp->zf) terp->eip += sex(p[1]);
  IF 117:                       /* jnz */
    terp->eip += 2;
    if (!terp->zf) terp->eip += sex(p[1]);
  IF 129:                       /* subtract immediate from %ebp */
    /* `sub` sets OF iff there is a signed overflow (a borrow from the
       bit next to the MSB into the MSB) and CF iff there is an
       unsigned overflow (a borrow from the MSB).  We currently aren’t
       using CF, but OF is necessary for `setge` to work. */
    req(p[1] == 237);
    terp->ebp -= u32_in(p+2);
    set_flags(terp, terp->ebp);
    terp->eip += 6;
  IF 133:                       /* test %eax, %eax */
    req(p[1] == 192);
    set_flags(terp, terp->eax);
    terp->eip += 2;
  IF 135:                       /* xchg %ebp, %esp */
    req(p[1] == 236);
    u32 tmp = terp->ebp;
    terp->ebp = terp->esp;
    terp->esp = tmp;
    terp->eip += 2;
  IF 136:                       /* mov %cl, (%eax) */
    req(p[1] == 8);
    *(u8*)translate(terp, terp->eax, 1) = terp->ecx;
    terp->eip += 2;
  IF 137:                       /* mov %esp, %ebp */
    req(p[1] == 229);
    terp->ebp = terp->esp;
    terp->eip += 2;
  IF 139:                       /* mov (%eax), %eax */
    req(p[1] == 0);
    terp->eax = u32_in(translate(terp, terp->eax, 4));
    terp->eip += 2;
  IF 143:                       /* popl (%eax) */
    req(p[1] == 0);
    u32 v;
    pop(terp, &v);
    u32_out(translate(terp, terp->eax, 4), v);
    terp->eip += 2;
  IF 184:                       /* load immediate %eax */
    terp->eax = u32_in(p+1);
    terp->eip += 5;
  IF 195:                       /* ret, no arguments */
    pop(terp, &terp->eip);
  IF 205:                       /* interrupt */
    req(p[1] == 0x80);          /* we only implement Linux! */
    terp->eip += 2;
    implement_syscall(terp, terp->eax, terp->ebx, terp->ecx, terp->edx);
  IF 232:                       /* call */
    {
      u32 old_eip = terp->eip;
      terp->eip += 5;
      terp->esp -= 4;
      u32_out(translate(terp, terp->esp, 4), terp->eip);
      terp->eip += sex_dword(u32_in(p+1)); /* XXX could this result in
                                              undefined signed
                                              overflow? Ecch */
      if (terp->tracing_stacks) trace("esp callsite callee",
                                      terp->esp, old_eip, terp->eip);
    }
  IF 254:                       /* dec %al */
    req(p[1] == 200);
    terp->eax = (terp->eax & 0xFfffFf00) | ((terp->eax & 0xff) - 1);
    set_flags(terp, terp->eax & 0xff); /* N.B. shouldn’t touch CF */
    terp->eip += 2;
  ELSE:
    die("Unimplemented instruction byte 0x%x at 0x%x", p[0], terp->eip);
  }
}

int main(int argc, char **argv)
{
  if (argc != 2) die("Usage: %s elffile", argv[0]);

  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) die("%s: open", argv[1]);
  off_t size = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);
  u8 *elf_file = malloc(size);
  if (!elf_file) die("malloc(%d)", size);
  ssize_t read_size = read(fd, elf_file, size);
  if (size != read_size) die("read → %d (!= %d)", size, read_size);

  terp_t terp;
  load(elf_file, read_size, &terp);
  terp.tracing_eip = 1;
  for (;;) {
    single_step(&terp);
  }
  return 0;
}
