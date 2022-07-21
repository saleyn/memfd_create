// vim:sw=2:ts=2:et
// Example illusrating how to load a shared object from memory and call
// its functions.

#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <asm/unistd.h>

// See: /usr/include/asm-generic/unistd.h
#ifndef __NR_memfd_create
#define __NR_memfd_create 319
#endif

// Wrapper for memfd_create syscall
inline int memfd_create(const char *name, unsigned int flags) {
  return syscall(__NR_memfd_create, name, flags);
}

// Returns: XYY kernel version, where X is major, and YY is minor (e.g. 317)
//         -1   if there is a problem determining kernel version
int get_kernel_version() {
  struct utsname buf;
  uname(&buf);

  fprintf(stderr, "Kernel version: %s\n", buf.release);

  char* saveptr;
  char* token  = strtok_r(buf.release, ".", &saveptr);
  if  (!token) return -1;
  int   major  = atoi(token) * 100;

  token = strtok_r(NULL, ".", &saveptr);
  if  (!token) return -1;
  int   minor  = atoi(token) % 100;

  return major + minor;
}

// Helper function to load an existing SO file into memory
void* load_file(char* filename, size_t* size) {
  FILE *f = fopen(filename, "rb");

  if (!f) {
    fprintf(stderr, "Could not open file %s: (%d) %s\n", filename, errno, strerror(errno));
    exit(1);
  }

  fseek(f, 0, SEEK_END);
  *size = ftell(f);
  fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

  char* buf = malloc(*size);
  if (!buf) {
    fprintf(stderr, "Could not allocate %ld bytes: (%d) %s\n", *size, errno, strerror(errno));
    exit(2);
  }

  fread(buf, *size, 1, f);
  fclose(f);

  return buf;
}

// Load a shared object from memory and associate it with a shm_fd file descriptor.
// The return is a handle to the shared memory object that is returned by the
// dlopen(2) function call.
//
// filename - filename to associate with a temporary in-memory file (can be NULL)
// mem      - memory containing the shared object image
// size     - size of the mem
void* dlopen_mem(const char* filename, void* mem, size_t size) {
  char  path[512];
  char  shm_name[512];

  int kernel_ver = get_kernel_version();
  if (kernel_ver < 0) {
    fprintf(stderr, "Could not determine kernel version\n");
    return NULL;
  }

  // Create some random shared memory file name
  if (!filename) {
    struct timeval tv;
    gettimeofday(&tv,NULL);

    snprintf(shm_name, sizeof(shm_name), "memfd-%ld", 1000000L * tv.tv_sec + tv.tv_usec);
    filename = shm_name;
  }

  // Get a file descriptor with a filename saved to `path`,
  // where a shared object can be written to in memory
  // suitable for a later call to dlopen. For Linux kernels < 317
  // set the path to the name of a shared memory file.
  // TODO: Generate temporary file name instead of using SHM_NAME
  int shm_fd = kernel_ver >= 317
             ? memfd_create(filename, 1)
             : shm_open(filename, O_RDWR | O_CREAT, S_IRWXU);

  if (shm_fd < 0) {
    fprintf(stderr, "Could not create mem file descriptor: (%d) %s\n", errno, strerror(errno));
    return NULL;
  }

  if (kernel_ver >= 317)
    snprintf(path, sizeof(path), "/proc/self/fd/%d", shm_fd);
  else
    snprintf(path, sizeof(path), "/dev/shm/%s", filename);

  if (write(shm_fd, mem, size) < 0) {
    fprintf(stderr, "Could not write %ld bytes to mem file: (%d) %s\n",
      size, errno, strerror(errno));
    close(shm_fd);
    return NULL;
  }

  void* handle = dlopen(path, RTLD_LAZY);
  if (!handle)
    fprintf(stderr,"Error in dlopen: %s\n", dlerror());

  close(shm_fd);
  unlink(path);

  return handle;
}

// Sample function from the shared object file
typedef int (*sample_function)();

int main (int argc, char **argv) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s file.so [shm_name]\n", argv[0]);
    exit(1);
  }

  size_t size;

  // First load the image of the so file into memory.
  // Any other method to get the file could be used here:
  // get a file from an Internet resource, from a zip
  // file, etc.
  void* mem = load_file(argv[1], &size);

  // This is the actual so loader from memory
  void* handle = dlopen_mem(argc == 3 ? argv[2] : NULL, mem, size);
  if (!handle)
    exit(2);

  printf("Shared object loaded from memory\n");

  // Map a function from the in-memory shared object
  const char*     fun = "sample_function";
  sample_function sym = (sample_function)dlsym(handle, fun);

  if (!sym || dlerror()) {
    fprintf(stderr, "Cannot find entry point '%s' in memory: %s", fun, dlerror());
    dlclose(handle);
    exit(3);
  }

  // Execute the function!!!
  int n = (*sym)();

  printf("SO function returned %d\n", n);

  dlclose(handle);

  if (n != 123)
    fprintf(stderr, "Invalid function return. Expected: 123. Got: %d\n", n);
}
