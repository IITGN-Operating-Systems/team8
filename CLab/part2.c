#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <math.h>
#include <time.h>

static size_t page_size;

// align_down - rounds a value down to an alignment
// @x: the value
// @a: the alignment (must be power of 2)
#define align_down(x, a) ((x) & ~((typeof(x))(a) - 1))

#define AS_LIMIT (1 << 25)    // Maximum limit on virtual memory bytes
#define MAX_SQRTS (1 << 27)   // Maximum limit on sqrt table entries
static double *sqrts;

// Global counters for quantitative measures.
static unsigned long total_accesses = 0;
static unsigned long page_faults = 0;
static unsigned long evictions = 0;

// Structure to track cached pages.
// Extended with frequency and insertion_time for new policies.
typedef struct {
  uintptr_t base;               // Base address of the page
  unsigned long last_accessed;  // For LRU: timestamp when last used.
  unsigned long insertion_time; // For FIFO: timestamp when inserted.
  unsigned long freq;           // For LFU: frequency counter.
} page_info;

static page_info *cache = NULL; // Array of cached pages
static int cache_count = 0;     // Number of pages currently cached
static int cache_slots = 0;     // Maximum number of pages in the cache
static unsigned long access_counter = 0; // Global counter for accesses

// Replacement algorithm types.
enum replacement_policy_t { 
    REPLACEMENT_LRU, 
    REPLACEMENT_MRU, 
    REPLACEMENT_FIFO,   // New: evict the oldest page inserted.
    REPLACEMENT_RANDOM, // New: evict a randomly selected page.
    REPLACEMENT_LFU     // New: evict the page with lowest access frequency.
};
static enum replacement_policy_t replacement_policy = REPLACEMENT_LRU;

// Use this helper function as an oracle for square root values.
static void
calculate_sqrts(double *sqrt_pos, int start, int nr)
{
  for (int i = 0; i < nr; i++)
    sqrt_pos[i] = sqrt((double)(start + i));
}

// Print current cache mapping.
static void print_cache()
{
    printf("Current cache mapping (%d/%d pages):\n", cache_count, cache_slots);
    for (int i = 0; i < cache_count; i++) {
        int start_index = (cache[i].base - (uintptr_t)sqrts) / sizeof(double);
        int num_entries = page_size / sizeof(double);
        printf("  Page %d: indices [%d - %d], last_accessed = %lu, insertion_time = %lu, freq = %lu\n",
               i, start_index, start_index + num_entries - 1,
               cache[i].last_accessed, cache[i].insertion_time, cache[i].freq);
    }
    printf("\n");
}

static void
handle_sigsegv(int sig, siginfo_t *si, void *ctx)
{
  uintptr_t fault_addr = (uintptr_t)si->si_addr;
  uintptr_t page_start = align_down(fault_addr, page_size);

  // Check if the faulting address is within the valid region.
  if (fault_addr < (uintptr_t)sqrts ||
      fault_addr >= (uintptr_t)sqrts + MAX_SQRTS * sizeof(double)) {
      fprintf(stderr, "oops got SIGSEGV at 0x%lx\n", fault_addr);
      exit(EXIT_FAILURE);
  }

  page_faults++;  // Count each SIGSEGV as a page fault.

  int victim_index = -1;
  if (cache_count >= cache_slots) {
      // Choose victim based on the replacement policy.
      switch(replacement_policy) {
          case REPLACEMENT_LRU: {
              unsigned long min = cache[0].last_accessed;
              victim_index = 0;
              for (int i = 1; i < cache_slots; i++) {
                  if (cache[i].last_accessed < min) {
                      min = cache[i].last_accessed;
                      victim_index = i;
                  }
              }
              break;
          }
          case REPLACEMENT_MRU: {
              unsigned long max = cache[0].last_accessed;
              victim_index = 0;
              for (int i = 1; i < cache_slots; i++) {
                  if (cache[i].last_accessed > max) {
                      max = cache[i].last_accessed;
                      victim_index = i;
                  }
              }
              break;
          }
          case REPLACEMENT_FIFO: {
              // Evict the page with the smallest insertion_time.
              unsigned long min = cache[0].insertion_time;
              victim_index = 0;
              for (int i = 1; i < cache_slots; i++) {
                  if (cache[i].insertion_time < min) {
                      min = cache[i].insertion_time;
                      victim_index = i;
                  }
              }
              break;
          }
          case REPLACEMENT_RANDOM: {
              // Choose a random victim.
              victim_index = rand() % cache_slots;
              break;
          }
          case REPLACEMENT_LFU: {
              // Evict the page with the lowest frequency.
              unsigned long min = cache[0].freq;
              victim_index = 0;
              for (int i = 1; i < cache_slots; i++) {
                  if (cache[i].freq < min) {
                      min = cache[i].freq;
                      victim_index = i;
                  }
              }
              break;
          }
          default:
              fprintf(stderr, "Unknown replacement policy.\n");
              exit(EXIT_FAILURE);
      }
      // Unmap the victim page.
      if (munmap((void *)cache[victim_index].base, page_size) == -1) {
          fprintf(stderr, "Couldn't munmap() page at 0x%lx; %s\n",
                  cache[victim_index].base, strerror(errno));
          exit(EXIT_FAILURE);
      }
      evictions++;  // Count an eviction.
      // Replace victim's record with new page mapping.
      cache[victim_index].base = page_start;
      cache[victim_index].last_accessed = ++access_counter;
      cache[victim_index].insertion_time = access_counter;  // For FIFO.
      cache[victim_index].freq = 1; // Reset frequency for LFU.
  } else {
      // Cache not full: add new page record.
      cache[cache_count].base = page_start;
      cache[cache_count].last_accessed = ++access_counter;
      cache[cache_count].insertion_time = access_counter;  // For FIFO.
      cache[cache_count].freq = 1; // For LFU.
      cache_count++;
  }

  // Map the new page at page_start.
  if (mmap((void *)page_start, page_size, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) == MAP_FAILED) {
      fprintf(stderr, "Couldn't mmap() page at 0x%lx; %s\n",
              page_start, strerror(errno));
      exit(EXIT_FAILURE);
  }

  // Initialize the page with square root values.
  int start_index = (page_start - (uintptr_t)sqrts) / sizeof(double);
  calculate_sqrts((double *)page_start, start_index, page_size / sizeof(double));

  // Print current cache info.
  print_cache();
}

// Set up the sqrt region and install SIGSEGV handler.
static void
setup_sqrt_region(void)
{
  struct rlimit lim = {AS_LIMIT, AS_LIMIT};
  struct sigaction act;

  sqrts = mmap(NULL, MAX_SQRTS * sizeof(double) + AS_LIMIT, PROT_NONE,
       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (sqrts == MAP_FAILED) {
    fprintf(stderr, "Couldn't mmap() region for sqrt table; %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (munmap(sqrts, MAX_SQRTS * sizeof(double) + AS_LIMIT) == -1) {
    fprintf(stderr, "Couldn't munmap() region for sqrt table; %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (setrlimit(RLIMIT_AS, &lim) == -1) {
    fprintf(stderr, "Couldn't set rlimit on RLIMIT_AS; %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  act.sa_sigaction = handle_sigsegv;
  act.sa_flags = SA_SIGINFO;
  sigemptyset(&act.sa_mask);
  if (sigaction(SIGSEGV, &act, NULL) == -1) {
    fprintf(stderr, "Couldn't set up SIGSEGV handler; %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

// Helper to print statistics.
static void print_stats(double elapsed_seconds)
{
    printf("===== Statistics =====\n");
    printf("Total accesses: %lu\n", total_accesses);
    printf("Page faults: %lu\n", page_faults);
    printf("Evictions: %lu\n", evictions);
    if (total_accesses > 0)
        printf("Page fault rate: %.2f%%\n", (double)page_faults * 100.0 / total_accesses);
    printf("Elapsed time: %.3f seconds\n", elapsed_seconds);
    if (total_accesses > 0)
        printf("Average time per access: %.6f microseconds\n",
               elapsed_seconds * 1e6 / total_accesses);
    printf("======================\n\n");
}

// Test routines.
static void
test_sqrt_region(void)
{
  int i, pos = rand() % (MAX_SQRTS - 1);
  double correct_sqrt;
  clock_t start, end;
  start = clock();

  printf("Validating square root table contents (mixed access)...\n");
  srand(0xDEADBEEF);

  for (i = 0; i < 500000; i++) {
    total_accesses++;
    if (i % 2 == 0)
      pos = rand() % (MAX_SQRTS - 1);
    else
      pos += 1;
    // For policies like LFU, simulate an access update:
    // (In a full implementation, you'd update frequency on each access if the page is resident.)
    printf("The number is %d\n", pos);
    calculate_sqrts(&correct_sqrt, pos, 1);
    if (sqrts[pos] != correct_sqrt) {
      fprintf(stderr, "Square root is incorrect. Expected %f, got %f.\n",
              correct_sqrt, sqrts[pos]);
      exit(EXIT_FAILURE);
    }
  }

  end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  printf("All tests passed!\n");
  print_stats(elapsed);
}

static void
test_sqrt_region_1(void)
{
  int i, pos;
  double correct_sqrt;
  clock_t start, end;
  start = clock();

  printf("Validating square root table contents (sequential access)...\n");

  for (i = 0; i < 50000000; i++) {
    total_accesses++;
    pos = i % (MAX_SQRTS - 1);
    calculate_sqrts(&correct_sqrt, pos, 1);
    if (sqrts[pos] != correct_sqrt) {
      fprintf(stderr, "Square root is incorrect. Expected %f, got %f.\n",
              correct_sqrt, sqrts[pos]);
      exit(EXIT_FAILURE);
    }
  }

  end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  printf("All tests passed for test_sqrt_region_1!\n");
  print_stats(elapsed);
}

static void
test_sqrt_region_1000(void)
{
  int i, pos;
  double correct_sqrt;
  clock_t start, end;
  start = clock();

  printf("Validating square root table contents (sparse access)...\n");

  for (i = 0; i < 50000000; i += 1000) {
    total_accesses++;
    pos = i % (MAX_SQRTS - 1);
    calculate_sqrts(&correct_sqrt, pos, 1);
    if (sqrts[pos] != correct_sqrt) {
      fprintf(stderr, "Square root is incorrect. Expected %f, got %f.\n",
              correct_sqrt, sqrts[pos]);
      exit(EXIT_FAILURE);
    }
  }

  end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
  printf("All tests passed for test_sqrt_region_1000!\n");
  print_stats(elapsed);
}

int
main(int argc, char *argv[])
{
  if (argc < 3) {
      fprintf(stderr, "Usage: %s <workload_type: 0,1,1000> <cache_slots> [lru|mru|fifo|random|lfu]\n", argv[0]);
      fprintf(stderr, "  Workload types:\n");
      fprintf(stderr, "    0 - mixed random/sequential\n");
      fprintf(stderr, "    1 - sequential\n");
      fprintf(stderr, "    1000 - sparse access\n");
      fprintf(stderr, "  Example: %s 0 16 lru\n", argv[0]);
      exit(EXIT_FAILURE);
  }

  int workload = atoi(argv[1]);
  cache_slots = atoi(argv[2]);
  if (cache_slots <= 0) {
      fprintf(stderr, "Cache slot size must be a positive integer.\n");
      exit(EXIT_FAILURE);
  }

  // Choose policy based on command-line argument if provided.
  if (argc >= 4) {
      if (strcmp(argv[3], "mru") == 0)
          replacement_policy = REPLACEMENT_MRU;
      else if (strcmp(argv[3], "fifo") == 0)
          replacement_policy = REPLACEMENT_FIFO;
      else if (strcmp(argv[3], "random") == 0)
          replacement_policy = REPLACEMENT_RANDOM;
      else if (strcmp(argv[3], "lfu") == 0)
          replacement_policy = REPLACEMENT_LFU;
      else  // default to LRU if not recognized.
          replacement_policy = REPLACEMENT_LRU;
  } else {
      // If no explicit policy is provided, choose one based on workload.
      if (workload == 0)
          replacement_policy = REPLACEMENT_LFU;    // Best for mixed access.
      else if (workload == 1)
          replacement_policy = REPLACEMENT_FIFO;   // Best for sequential.
      else if (workload == 1000)
          replacement_policy = REPLACEMENT_RANDOM; // Best for sparse access.
      else
          replacement_policy = REPLACEMENT_LRU;
  }

  // Allocate memory for the cache.
  cache = malloc(sizeof(page_info) * cache_slots);
  if (!cache) {
      fprintf(stderr, "Failed to allocate memory for cache.\n");
      exit(EXIT_FAILURE);
  }

  page_size = sysconf(_SC_PAGESIZE);
  printf("page_size is %ld\n", page_size);
  printf("Cache slots: %d, Replacement policy: ", cache_slots);
  switch (replacement_policy) {
      case REPLACEMENT_LRU:    printf("LRU\n\n"); break;
      case REPLACEMENT_MRU:    printf("MRU\n\n"); break;
      case REPLACEMENT_FIFO:   printf("FIFO\n\n"); break;
      case REPLACEMENT_RANDOM: printf("RANDOM\n\n"); break;
      case REPLACEMENT_LFU:    printf("LFU\n\n"); break;
      default:                 printf("Unknown\n\n"); break;
  }
  setup_sqrt_region();

  // Reset counters before each test.
  total_accesses = page_faults = evictions = access_counter = 0;
  cache_count = 0;

  switch (workload) {
    case 1:
      test_sqrt_region_1();
      break;
    case 1000:
      test_sqrt_region_1000();
      break;
    default:
      test_sqrt_region();
      break;
  }

  free(cache);
  return 0;
}