#include <elf.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "hypercall.h"
#include "../iai_common.h"

/* CR0 bits */
#define CR0_PE 1u
#define CR0_MP (1U << 1)
#define CR0_EM (1U << 2)
#define CR0_TS (1U << 3)
#define CR0_ET (1U << 4)
#define CR0_NE (1U << 5)
#define CR0_WP (1U << 16)
#define CR0_AM (1U << 18)
#define CR0_NW (1U << 29)
#define CR0_CD (1U << 30)
// enable paging
#define CR0_PG (1U << 31)

/* CR4 bits */
#define CR4_VME 1
#define CR4_PVI (1U << 1)
#define CR4_TSD (1U << 2)
#define CR4_DE (1U << 3)
#define CR4_PSE (1U << 4)
#define CR4_PAE (1U << 5)
#define CR4_MCE (1U << 6)
#define CR4_PGE (1U << 7)
#define CR4_PCE (1U << 8)
#define CR4_OSFXSR (1U << 9)
#define CR4_OSXMMEXCPT (1U << 10)
#define CR4_UMIP (1U << 11)
#define CR4_VMXE (1U << 13)
#define CR4_SMXE (1U << 14)
#define CR4_FSGSBASE (1U << 16)
#define CR4_PCIDE (1U << 17)
#define CR4_OSXSAVE (1U << 18)
#define CR4_SMEP (1U << 20)
#define CR4_SMAP (1U << 21)

#define EFER_SCE 1
#define EFER_LME (1U << 8)
#define EFER_LMA (1U << 10)
// No-Execute switch
#define EFER_NXE (1U << 11)

/* 64-bit page * entry bits */
#define PDE64_PRESENT 1
#define PDE64_RW (1U << 1)
#define PDE64_USER (1U << 2)
#define PDE64_ACCESSED (1U << 5)
#define PDE64_DIRTY (1U << 6)
#define PDE64_PS (1U << 7)
#define PDE64_G (1U << 8)
// 4KB page No-Execute lock
#define PTE64_NX (1ULL << 63)

// usage message
#define USAGE(bin)                                                             \
  fprintf(stderr, "Usage: %s [-m memory_in_MB] [-v] <binary_file>\n", bin)

// default memory size 2MB
#define MB (1024 * 1024)
#define DEFAULT_MEM_SIZE (2 * MB)

// page size 4KB
#define PAGE_SIZE 4096
// root page table physical address
#define PML4_ADDR 0x1000
// entry point for elf binary
#define ELF_ENTRY_POINT 0x100000

// ports for communication
#define SINGLE_BYTE_PORT 0xE9

int iai_debug = 0;

struct vm {
  int sys_fd;
  int fd;
  char *mem;
  size_t mem_size;
  // root page table physical address
  uint64_t pml4_addr;
};

/**
 * @brief Initialize virtual memory.
 */
void vm_init(struct vm *vm, size_t mem_size) {
  int api_ver;
  struct kvm_userspace_memory_region memreg;

  vm->mem_size = mem_size;

  vm->sys_fd = open("/dev/kvm", O_RDWR);
  if (vm->sys_fd < 0) {
    perror("open /dev/kvm");
    exit(1);
  }

  api_ver = ioctl(vm->sys_fd, KVM_GET_API_VERSION, 0);
  if (api_ver < 0) {
    perror("KVM_GET_API_VERSION");
    exit(1);
  }

  if (api_ver != KVM_API_VERSION) {
    fprintf(stderr, "Got KVM api version %d, expected %d\n", api_ver,
            KVM_API_VERSION);
    exit(1);
  }

  vm->fd = ioctl(vm->sys_fd, KVM_CREATE_VM, 0);
  if (vm->fd < 0) {
    perror("KVM_CREATE_VM");
    exit(1);
  }

  if (ioctl(vm->fd, KVM_SET_TSS_ADDR, 0xfffbd000) < 0) {
    perror("KVM_SET_TSS_ADDR");
    exit(1);
  }

  vm->mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (vm->mem == MAP_FAILED) {
    perror("mmap mem");
    exit(1);
  }

  madvise(vm->mem, mem_size, MADV_MERGEABLE);

  memreg.slot = 0;
  memreg.flags = 0;
  memreg.guest_phys_addr = 0;
  memreg.memory_size = mem_size;
  memreg.userspace_addr = (unsigned long)vm->mem;
  if (ioctl(vm->fd, KVM_SET_USER_MEMORY_REGION, &memreg) < 0) {
    perror("KVM_SET_USER_MEMORY_REGION");
    exit(1);
  }
}

uint64_t next_free_table_addr = PML4_ADDR;

/**
 * @brief Allocate a new table which is size of page.
 */
uint64_t allocate_table_page(struct vm *vm) {
  uint64_t addr = next_free_table_addr;
  next_free_table_addr += PAGE_SIZE;
  if (next_free_table_addr >= ELF_ENTRY_POINT) {
    fprintf(stderr,
            "Fatal: Page tables grew too large and hit ELF entry point\n");
    exit(1);
  }

  // zero out the new table
  memset(vm->mem + addr, 0, PAGE_SIZE);
  return addr;
}

/**
 * @brief Map a new 4K page for the physical address. Implement W^X hardware
 * security.
 */
void map_4k_page(struct vm *vm, uint64_t phys_addr, int writeable,
                 int executable) {
  // get physical address of page table root
  uint64_t *pml4 = (uint64_t *)(vm->mem + vm->pml4_addr);

  // extract index from physical address
  int pml4_idx = (phys_addr >> 39) & 0x1FF;
  int pdpt_idx = (phys_addr >> 30) & 0x1FF;
  int pd_idx = (phys_addr >> 21) & 0x1FF;
  int pt_idx = (phys_addr >> 12) & 0x1FF;

  if (!(pml4[pml4_idx] & PDE64_PRESENT)) {
    // add new entry to pml4
    pml4[pml4_idx] =
        allocate_table_page(vm) | PDE64_PRESENT | PDE64_RW | PDE64_USER;
  }

  uint64_t *pdpt = (uint64_t *)(vm->mem + (pml4[pml4_idx] & ~0xFFFULL));
  if (!(pdpt[pdpt_idx] & PDE64_PRESENT)) {
    // add new entry to pdpt
    pdpt[pdpt_idx] =
        allocate_table_page(vm) | PDE64_PRESENT | PDE64_RW | PDE64_USER;
  }

  uint64_t *pd = (uint64_t *)(vm->mem + (pdpt[pdpt_idx] & ~0xFFFULL));
  if (!(pd[pd_idx] & PDE64_PRESENT)) {
    // add new entry to pd
    pd[pd_idx] =
        allocate_table_page(vm) | PDE64_PRESENT | PDE64_RW | PDE64_USER;
  }

  uint64_t *pt = (uint64_t *)(vm->mem + (pd[pd_idx] & ~0xFFFULL));
  // construct the final entry with granular permissions
  uint64_t entry = phys_addr | PDE64_PRESENT | PDE64_USER;
  if (writeable)
    entry |= PDE64_RW;
  if (!executable)
    entry |= PTE64_NX;

  // commit to the hardware page table
  pt[pt_idx] = entry;
}

void init_page_tables(struct vm *vm) {
  // allocate root page table
  vm->pml4_addr = allocate_table_page(vm);
  // map all virtual memory as writable but not executable
  for (uint64_t addr = 0; addr < vm->mem_size; addr += PAGE_SIZE) {
    map_4k_page(vm, addr, 1, 0);
  }
}

struct vcpu {
  int fd;
  struct kvm_run *kvm_run;
};

/**
 * @brief Initialize virtual CPU.
 */
void vcpu_init(struct vm *vm, struct vcpu *vcpu) {
  int vcpu_mmap_size;

  vcpu->fd = ioctl(vm->fd, KVM_CREATE_VCPU, 0);
  if (vcpu->fd < 0) {
    perror("KVM_CREATE_VCPU");
    exit(1);
  }

  vcpu_mmap_size = ioctl(vm->sys_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
  if (vcpu_mmap_size <= 0) {
    perror("KVM_GET_VCPU_MMAP_SIZE");
    exit(1);
  }

  vcpu->kvm_run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                       vcpu->fd, 0);
  if (vcpu->kvm_run == MAP_FAILED) {
    perror("mmap kvm_run");
    exit(1);
  }
}

/**
 * @brief Run the virtual machine.
 */
int run_vm(struct vcpu *vcpu, char *mem) {
  uint32_t msg_buf_addr = 0;
  struct kvm_run *run = vcpu->kvm_run;

  for (;;) {
    if (ioctl(vcpu->fd, KVM_RUN, 0) < 0) {
      perror("KVM_RUN");
      exit(1);
    }

    switch (run->exit_reason) {
    case KVM_EXIT_HLT:
      if (iai_debug) {
        fprintf(stderr, "[Host] Guest execution completed cleanly.\n");
      }
      return 0;

    case KVM_EXIT_IO:
      if (run->io.direction == KVM_EXIT_IO_OUT) {
        if (run->io.port == SINGLE_BYTE_PORT) {
          // single byte
          char *p = (char *)run + run->io.data_offset;
          putchar(*p);
        } else if (run->io.port == SHM_ADDR_PORT) {
          // get address of message buffer
          msg_buf_addr = *(uint32_t *)((char *)run + run->io.data_offset);
        } else if (run->io.port == HYPERCALL_PORT) {
          handle_hypercall(mem, msg_buf_addr);
        } else if (run->io.port == SHM_LEN_PORT) {
          // Backward compatibility for old write
          uint32_t length = *(uint32_t *)((char *)run + run->io.data_offset);
          char *shared_buffer = (char *)mem + msg_buf_addr;
          fwrite(shared_buffer, 1, length, stdout);
          fflush(stdout);
        }
      }
      continue;

    default:
      fprintf(stderr,
              "Got exit_reason %d,"
              " expected KVM_EXIT_HLT (%d)\n",
              vcpu->kvm_run->exit_reason, KVM_EXIT_HLT);
      exit(1);
    }
  }
}

/**
 * @brief Setup code segment register.
 */
static void setup_64bit_code_segment(struct kvm_sregs *sregs) {
  struct kvm_segment seg = {
      .base = 0,
      .limit = 0xffffffff,
      .selector = 1 << 3,
      .present = 1,
      .type = 11, /* Code: execute, read, accessed */
      .dpl = 0,
      .db = 0,
      .s = 1, /* Code/data */
      .l = 1,
      .g = 1, /* 4KB granularity */
  };

  sregs->cs = seg;

  seg.type = 3; /* Data: read/write, accessed */
  seg.selector = 2 << 3;
  sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}

/**
 * @brief Setup special registers for long mode.
 */
static void setup_long_mode(struct vm *vm, struct kvm_sregs *sregs) {
  sregs->cr3 = vm->pml4_addr;
  // CR4_PAE is required for long mode.
  // CR4_OSFXSR and CR4_OSXMMEXCPT enable SSE (XMM registers).
  // GCC often generates these instructions for optimizations at -O2.
  sregs->cr4 = CR4_PAE | CR4_OSFXSR | CR4_OSXMMEXCPT;
  // enable paging
  sregs->cr0 = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;
  // activate NXE
  sregs->efer = EFER_LME | EFER_LMA | EFER_NXE;

  setup_64bit_code_segment(sregs);
}

/**
 * @brief Run VM in long mode.
 */
int run_long_mode(struct vm *vm, struct vcpu *vcpu, uint64_t entry_point) {
  struct kvm_sregs sregs;
  struct kvm_regs regs;

  if (ioctl(vcpu->fd, KVM_GET_SREGS, &sregs) < 0) {
    perror("KVM_GET_SREGS");
    exit(1);
  }

  setup_long_mode(vm, &sregs);

  if (ioctl(vcpu->fd, KVM_SET_SREGS, &sregs) < 0) {
    perror("KVM_SET_SREGS");
    exit(1);
  }

  memset(&regs, 0, sizeof(regs));
  /* Clear all FLAGS bits, except bit 1 which is always set. */
  regs.rflags = 2;
  // set RIP to the entry point from elf loader
  regs.rip = entry_point;
  // create stack at top of reserved memory
  regs.rsp = vm->mem_size;

  if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0) {
    perror("KVM_SET_REGS");
    exit(1);
  }

  return run_vm(vcpu, vm->mem);
}

/**
 * @brief Parse an ELF binary and load its segments into VM memory.
 * @return The execution entry point (RIP) defined in the ELF file.
 */
uint64_t load_elf(struct vm *vm, const char *filename) {
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    perror("Failed to open ELF file");
    exit(1);
  }

  // Read the ELF Header
  Elf64_Ehdr ehdr;
  if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr)) {
    perror("Failed to read ELF header");
    exit(1);
  }

  // Verify it is actually an ELF file
  if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
    fprintf(stderr, "Error: File is not a valid ELF binary.\n");
    exit(1);
  }

  // Iterate through the Program Headers to find loadable segments
  Elf64_Phdr phdr;
  for (int i = 0; i < ehdr.e_phnum; i++) {
    // Move to the offset of the current program header
    lseek(fd, ehdr.e_phoff + (i * sizeof(phdr)), SEEK_SET);
    read(fd, &phdr, sizeof(phdr));

    // We only care about PT_LOAD segments (actual code/data to map to RAM)
    if (phdr.p_type == PT_LOAD) {
      if (phdr.p_paddr + phdr.p_memsz > vm->mem_size) {
        fprintf(stderr, "Error: ELF segment exceeds VM memory size.\n");
        exit(1);
      }

      // Move to where the actual segment data is located in the file
      lseek(fd, phdr.p_offset, SEEK_SET);

      // Copy the data into the VM's physical memory space
      if (phdr.p_filesz > 0) {
        read(fd, vm->mem + phdr.p_paddr, phdr.p_filesz);
      }

      // Zero out the BSS section (uninitialized variables)
      // If the memory size is larger than the file size, the remainder must be
      // zeros
      if (phdr.p_memsz > phdr.p_filesz) {
        memset(vm->mem + phdr.p_paddr + phdr.p_filesz, 0,
               phdr.p_memsz - phdr.p_filesz);
      }

      // read ELF flags to implement W^X security
      int is_writeable = (phdr.p_flags & PF_W) != 0;
      int is_executable = (phdr.p_flags & PF_X) != 0;

      // ensure we hit exact 4KB page boundaries when locking down the segment
      uint64_t start_page = phdr.p_paddr & ~0xFFFULL;
      uint64_t end_page = (phdr.p_paddr + phdr.p_memsz + 0xFFF) & ~0xFFFULL;
      // update the permissions for the pages in this segment
      for (uint64_t addr = start_page; addr < end_page; addr += PAGE_SIZE) {
        map_4k_page(vm, addr, is_writeable, is_executable);
      }

      if (iai_debug) {
        fprintf(stderr,
                "[ELF Loader] Segment loaded at 0x%lx | Size: %ld | W=%d X=%d\n",
                (unsigned long)phdr.p_paddr, (long)phdr.p_memsz, is_writeable,
                is_executable);
      }
    }
  }

  close(fd);
  return ehdr.e_entry;
}

/**
 * @brief Load binary to run in the VM.
 */
void load_flat_binary(struct vm *vm, const char *filename,
                      size_t max_mem_size) {
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    perror("Failed to open binary file");
    exit(1);
  }

  // get the size of the file
  struct stat st;
  if (fstat(fd, &st) < 0) {
    perror("Failed to stat binary file");
    close(fd);
    exit(1);
  }

  // ensure binary is not bigger than VM's physical memory
  if ((size_t)st.st_size > max_mem_size) {
    fprintf(stderr, "Error: Binary size (%lld) exceeds VM memory size (%zu)\n",
            (long long)st.st_size, max_mem_size);
    close(fd);
    exit(1);
  }

  ssize_t bytes_read = read(fd, vm->mem, st.st_size);
  if (bytes_read < 0 || bytes_read != st.st_size) {
    perror("Failed to read entire binary into guest memory");
    close(fd);
    exit(1);
  }

  fprintf(stderr,
          "Successfully loaded %zd bytes from '%s' into guest memory.\n",
          bytes_read, filename);
  close(fd);
}

int main(int argc, char **argv) {
  size_t mem_size = DEFAULT_MEM_SIZE;
  int opt;

  init_fd_map();

  while ((opt = getopt(argc, argv, "m:v")) != -1) {
    switch (opt) {
    case 'm':
      mem_size = (size_t)strtoull(optarg, NULL, 10) * MB;
      if (mem_size <= 0) {
        fprintf(stderr, "Error: Invalid memory size.\n");
        return 1;
      }
      break;
    case 'v':
      iai_debug = 1;
      break;
    default:
      USAGE(argv[0]);
      return 1;
    }
  }

  if (optind >= argc) {
    fprintf(stderr, "Error: Missing required binary file.\n");
    USAGE(argv[0]);
    return 1;
  }

  const char *bin_filename = argv[optind];

  if (iai_debug) {
    fprintf(stderr, "Starting VM with %zu bytes of memory...\n", mem_size);
  }

  struct vm vm;
  struct vcpu vcpu;

  vm_init(&vm, mem_size);
  // initialize page tables
  init_page_tables(&vm);

  // load ELF binary
  uint64_t entry_point = load_elf(&vm, bin_filename);

  // initialize VCPU
  vcpu_init(&vm, &vcpu);

  // run the VM
  return run_long_mode(&vm, &vcpu, entry_point);
}
