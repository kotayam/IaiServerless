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
#define CR4_OSFXSR (1U << 8)
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
#define EFER_NXE (1U << 11)

/* 64-bit page * entry bits */
#define PDE64_PRESENT 1
#define PDE64_RW (1U << 1)
#define PDE64_USER (1U << 2)
#define PDE64_ACCESSED (1U << 5)
#define PDE64_DIRTY (1U << 6)
#define PDE64_PS (1U << 7)
#define PDE64_G (1U << 8)

// usage message
#define USAGE(bin)                                                             \
  fprintf(stderr, "Usage: %s [-m memory_in_MB] <binary_file>\n", bin)

// default memory size 2MB
#define MB (1024 * 1024)
#define DEFAULT_MEM_SIZE (2 * MB)

struct vm {
  int sys_fd;
  int fd;
  char *mem;
};

void vm_init(struct vm *vm, size_t mem_size) {
  int api_ver;
  struct kvm_userspace_memory_region memreg;

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

struct vcpu {
  int fd;
  struct kvm_run *kvm_run;
};

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

int run_vm(struct vcpu *vcpu) {
  for (;;) {
    if (ioctl(vcpu->fd, KVM_RUN, 0) < 0) {
      perror("KVM_RUN");
      exit(1);
    }

    switch (vcpu->kvm_run->exit_reason) {
    case KVM_EXIT_HLT:
      fprintf(stderr, "[Host] Guest execution completed cleanly.\n");
      return 0;

    case KVM_EXIT_IO:
      // forward binary I/O write to stdout
      if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT &&
          vcpu->kvm_run->io.port == 0xE9) {
        char *p = (char *)vcpu->kvm_run;
        fwrite(p + vcpu->kvm_run->io.data_offset, vcpu->kvm_run->io.size, 1,
               stdout);
        fflush(stdout);
        continue;
      }

      /* fall through */
    default:
      fprintf(stderr,
              "Got exit_reason %d,"
              " expected KVM_EXIT_HLT (%d)\n",
              vcpu->kvm_run->exit_reason, KVM_EXIT_HLT);
      exit(1);
    }
  }
}

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

static void setup_long_mode(struct vm *vm, struct kvm_sregs *sregs) {
  uint64_t pml4_addr = 0x2000;
  uint64_t *pml4 = (void *)(vm->mem + pml4_addr);

  uint64_t pdpt_addr = 0x3000;
  uint64_t *pdpt = (void *)(vm->mem + pdpt_addr);

  uint64_t pd_addr = 0x4000;
  uint64_t *pd = (void *)(vm->mem + pd_addr);

  pml4[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pdpt_addr;
  pdpt[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pd_addr;
  pd[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | PDE64_PS;

  sregs->cr3 = pml4_addr;
  sregs->cr4 = CR4_PAE;
  sregs->cr0 = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;
  sregs->efer = EFER_LME | EFER_LMA;

  setup_64bit_code_segment(sregs);
}

int run_long_mode(struct vm *vm, struct vcpu *vcpu) {
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
  regs.rip = 0;
  /* Create stack at top of 2 MB page and grow down. */
  regs.rsp = 2 << 20;

  if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0) {
    perror("KVM_SET_REGS");
    exit(1);
  }

  return run_vm(vcpu);
}

/**
 * @brief Load binary to run in the VM.
 */
void load_binary(struct vm *vm, const char *filename, size_t max_mem_size) {
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

  while ((opt = getopt(argc, argv, "m:")) != -1) {
    switch (opt) {
    case 'm':
      mem_size = (size_t)strtoull(optarg, NULL, 10) * MB;
      if (mem_size <= 0) {
        fprintf(stderr, "Error: Invalid memory size.\n");
        return 1;
      }
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

  fprintf(stderr, "Starting VM with %zu bytes of memory...\n", mem_size);

  struct vm vm;
  struct vcpu vcpu;
  vm_init(&vm, mem_size);
  load_binary(&vm, bin_filename, mem_size);
  vcpu_init(&vm, &vcpu);

  // run the VM
  return run_long_mode(&vm, &vcpu);
}
