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

// ************************* Global definitions and region setup *******************************

// Align a value down to the nearest multiple of a given alignment.
// Note: 'a' must be a power of 2.
#define align_down(x, a) ((x) & ~((typeof(x))(a) - 1))

#define AS_LIMIT (1 << 25)  // Maximum virtual memory limit in bytes
#define MAX_SQRTS (1 << 27) // Maximum number of entries in the sqrt table

static double *sqrts; // Pointer to the base of the square root table
static size_t page_size;

// Static Variables for Statistics
static unsigned long total_accesses = 0;
static unsigned long num_faults = 0;
static unsigned long evictions = 0;

// ---------------------------------------------------------------------
// Replacement policy types
// ---------------------------------------------------------------------
enum replacement_policy_t
{
  REPLACEMENT_LRU,    // Least Recently Used (uses a doubly-linked list)
  REPLACEMENT_FIFO,   // First In, First Out (uses a queue-like structure)
  REPLACEMENT_MRU,    // Most Recently Used
  REPLACEMENT_RANDOM, // Random replacement
  REPLACEMENT_LFU     // Least Frequently Used
};
static enum replacement_policy_t replacement_policy = REPLACEMENT_LRU;

// ---------------------------------------------------------------------
// Linked-list node structure for LRU and FIFO policies
// ---------------------------------------------------------------------
typedef struct cache_node
{
  uintptr_t base;          // Base address of the mapped page
  struct cache_node *prev; // Pointer to the previous node
  struct cache_node *next; // Pointer to the next node
} cache_node;

static cache_node *list_head = NULL; // Head of the linked list
static cache_node *list_tail = NULL; // Tail of the linked list
static int list_size = 0;            // Current number of pages in the list
static int cache_slots = 0;          // Maximum number of pages allowed in the cache

// ---------------------------------------------------------------------
// Helper function to calculate square roots for a range of numbers
// Fills 'nr' entries starting at 'sqrt_pos' with sqrt(start + i)
// ---------------------------------------------------------------------
static void calculate_sqrts(double *sqrt_pos, int start, int nr)
{
  for (int i = 0; i < nr; i++)
    sqrt_pos[i] = sqrt((double)(start + i));
}

// ---------------------------------------------------------------------
// Linked-list helper functions for LRU and FIFO policies
// ---------------------------------------------------------------------

// Move a node to the front of the list (used in LRU policy)
static void lru_move_to_front(cache_node *node)
{
  if (node == list_head)
    return; // Node is already at the front

  // Detach the node from its current position
  if (node->prev)
    node->prev->next = node->next;
  if (node->next)
    node->next->prev = node->prev;
  if (node == list_tail)
    list_tail = node->prev;

  // Insert the node at the front of the list
  node->prev = NULL;
  node->next = list_head;
  if (list_head)
    list_head->prev = node;
  list_head = node;
  if (list_tail == NULL)
    list_tail = node;
}

// Insert a new page at the front of the list (used in LRU policy)
// If the cache is full, evict the least recently used page
static void lru_insert(uintptr_t page_start)
{
  if (list_size >= cache_slots)
  {
    // Evict the least recently used page (tail of the list)
    cache_node *victim = list_tail;
    if (munmap((void *)victim->base, page_size) == -1)
    {
      fprintf(stderr, "Error unmapping page at 0x%lx: %s\n",
              victim->base, strerror(errno));
      exit(EXIT_FAILURE);
    }
    evictions++;
    // Remove the victim node from the list
    if (victim->prev)
    {
      victim->prev->next = NULL;
      list_tail = victim->prev;
    }
    else
    {
      // Only one element in the list
      list_head = list_tail = NULL;
    }
    free(victim);
    list_size--;
  }

  // Map the new page at the specified address
  if (mmap((void *)page_start, page_size, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) == MAP_FAILED)
  {
    fprintf(stderr, "Error mapping page at 0x%lx: %s\n",
            page_start, strerror(errno));
    exit(EXIT_FAILURE);
  }
  // Initialize the new page with square root values
  int start_index = (page_start - (uintptr_t)sqrts) / sizeof(double);
  calculate_sqrts((double *)page_start, start_index, page_size / sizeof(double));

  // Create a new node and add it to the front of the list
  cache_node *node = malloc(sizeof(cache_node));
  if (!node)
  {
    fprintf(stderr, "Memory allocation failed for cache node\n");
    exit(EXIT_FAILURE);
  }
  node->base = page_start;
  node->prev = NULL;
  node->next = list_head;
  if (list_head)
    list_head->prev = node;
  list_head = node;
  if (list_tail == NULL)
    list_tail = node;
  list_size++;
}

// Insert a new page at the end of the list (used in FIFO policy)
// If the cache is full, evict the oldest page
static void fifo_insert(uintptr_t page_start)
{
  if (list_size >= cache_slots)
  {
    // Evict the oldest page (head of the list)
    cache_node *victim = list_head;
    if (munmap((void *)victim->base, page_size) == -1)
    {
      fprintf(stderr, "Error unmapping page at 0x%lx: %s\n",
              victim->base, strerror(errno));
      exit(EXIT_FAILURE);
    }
    evictions++;
    // Remove the head node from the list
    list_head = victim->next;
    if (list_head)
      list_head->prev = NULL;
    else
      list_tail = NULL;
    free(victim);
    list_size--;
  }

  // Map the new page at the specified address
  if (mmap((void *)page_start, page_size, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) == MAP_FAILED)
  {
    fprintf(stderr, "Error mapping page at 0x%lx: %s\n",
            page_start, strerror(errno));
    exit(EXIT_FAILURE);
  }
  // Initialize the new page with square root values
  int start_index = (page_start - (uintptr_t)sqrts) / sizeof(double);
  calculate_sqrts((double *)page_start, start_index, page_size / sizeof(double));

  // Create a new node and add it to the end of the list
  cache_node *node = malloc(sizeof(cache_node));
  if (!node)
  {
    fprintf(stderr, "Memory allocation failed for cache node\n");
    exit(EXIT_FAILURE);
  }
  node->base = page_start;
  node->next = NULL;
  node->prev = list_tail;
  if (list_tail)
    list_tail->next = node;
  list_tail = node;
  if (!list_head)
    list_head = node;
  list_size++;
}

// Helper function to print the current state of the cache
static void print_list_cache()
{
  printf("Cache state (linked list: %d/%d):\n", list_size, cache_slots);
  cache_node *curr = list_head;
  int idx = 0;
  while (curr)
  {
    int start_index = (curr->base - (uintptr_t)sqrts) / sizeof(double);
    int num_entries = page_size / sizeof(double);
    printf("  Node %d: indices [%d - %d]\n", idx, start_index, start_index + num_entries - 1);
    curr = curr->next;
    idx++;
  }
  printf("\n");
}

// ---------------------------------------------------------------------
// SIGSEGV signal handler: Handles page faults and invokes the replacement
// policy to map or remap the faulting page
// ---------------------------------------------------------------------
static void handle_sigsegv(int sig, siginfo_t *si, void *ctx)
{
  uintptr_t fault_addr = (uintptr_t)si->si_addr;
  uintptr_t page_start = align_down(fault_addr, page_size);

  // Check if the fault address is within the valid sqrt region
  if (fault_addr < (uintptr_t)sqrts ||
      fault_addr >= (uintptr_t)sqrts + MAX_SQRTS * sizeof(double))
  {
    fprintf(stderr, "Unexpected SIGSEGV at 0x%lx\n", fault_addr);
    exit(EXIT_FAILURE);
  }

  num_faults++; // Increment the fault counter

  // Handle the fault based on the replacement policy
  if (replacement_policy == REPLACEMENT_LRU)
  {
    // Check if the page is already in the cache
    cache_node *curr = list_head;
    while (curr)
    {
      if (curr->base == page_start)
      {
        lru_move_to_front(curr);
        return;
      }
      curr = curr->next;
    }
    // Page not found, insert it using LRU policy
    lru_insert(page_start);
    print_list_cache();
  }
  else if (replacement_policy == REPLACEMENT_FIFO)
  {
    // Check if the page is already in the cache
    cache_node *curr = list_head;
    while (curr)
    {
      if (curr->base == page_start)
        return; // No need to update order for FIFO
      curr = curr->next;
    }
    // Page not found, insert it using FIFO policy
    fifo_insert(page_start);
    print_list_cache();
  }
  else
  {
    // For unsupported policies, print an error and exit
    fprintf(stderr, "Unsupported replacement policy.\n");
    exit(EXIT_FAILURE);
  }
}

// ---------------------------------------------------------------------
// Set up the sqrt region and install the SIGSEGV handler
// ---------------------------------------------------------------------
static void setup_sqrt_region(void)
{
  struct rlimit lim = {AS_LIMIT, AS_LIMIT};
  struct sigaction act;

  // Reserve the memory region for the sqrt table
  sqrts = mmap(NULL, MAX_SQRTS * sizeof(double) + AS_LIMIT, PROT_NONE,
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (sqrts == MAP_FAILED)
  {
    fprintf(stderr, "Error reserving memory for sqrt table: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (munmap(sqrts, MAX_SQRTS * sizeof(double) + AS_LIMIT) == -1)
  {
    fprintf(stderr, "Error unmapping memory for sqrt table: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Set the soft limit on virtual memory usage
  if (setrlimit(RLIMIT_AS, &lim) == -1)
  {
    fprintf(stderr, "Error setting memory limit: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Install the SIGSEGV handler
  act.sa_sigaction = handle_sigsegv;
  act.sa_flags = SA_SIGINFO;
  sigemptyset(&act.sa_mask);
  if (sigaction(SIGSEGV, &act, NULL) == -1)
  {
    fprintf(stderr, "Error setting up SIGSEGV handler: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

// ---------------------------------------------------------------------
// Statistics printer
// ---------------------------------------------------------------------
static void print_stats(double elapsed_seconds)
{
  printf("===== Statistics =====\n");
  printf("Total accesses: %lu\n", total_accesses);
  printf("Page faults: %lu\n", num_faults);
  printf("Evictions: %lu\n", evictions);
  if (total_accesses > 0)
    printf("Fault rate: %.2f%%\n", (double)num_faults * 100.0 / total_accesses);
  printf("Elapsed time: %.3f seconds\n", elapsed_seconds);
  if (total_accesses > 0)
    printf("Avg time per access: %.6f microseconds\n",
           elapsed_seconds * 1e6 / total_accesses);
  printf("======================\n\n");
}

// ---------------------------------------------------------------------
// Test routines
// ---------------------------------------------------------------------

static void
test_sqrt_region(void)
{
  int i, pos = rand() % (MAX_SQRTS - 1);
  double correct_sqrt;
  clock_t start, end;
  start = clock();

  printf("Validating square root table contents (mixed access)...\n");
  srand(0xDEADBEEF);

  for (i = 0; i < 500000; i++)
  {
    total_accesses++;
    if (i % 2 == 0)
      pos = rand() % (MAX_SQRTS - 1);
    else
      pos += 1;
    printf("The number is %d\n", pos);
    calculate_sqrts(&correct_sqrt, pos, 1);
    if (sqrts[pos] != correct_sqrt)
    {
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

  for (i = 0; i < 50000000; i++)
  {
    total_accesses++;
    pos = i % (MAX_SQRTS - 1);
    calculate_sqrts(&correct_sqrt, pos, 1);
    if (sqrts[pos] != correct_sqrt)
    {
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

  for (i = 0; i < 50000000; i += 1000)
  {
    total_accesses++;
    pos = i % (MAX_SQRTS - 1);
    calculate_sqrts(&correct_sqrt, pos, 1);
    if (sqrts[pos] != correct_sqrt)
    {
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

// ---------------------------------------------------------------------
// Entry Point
// ---------------------------------------------------------------------

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    fprintf(stderr, "Usage: %s <workload_type: 0,1,1000> <cache_slots> [lru|fifo|mru|random|lfu]\n", argv[0]);
    fprintf(stderr, "  Workload types:\n");
    fprintf(stderr, "    0 - mixed random/sequential\n");
    fprintf(stderr, "    1 - sequential\n");
    fprintf(stderr, "    1000 - sparse access\n");
    fprintf(stderr, "Example: %s 0 16 lru\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int workload = atoi(argv[1]);
  cache_slots = atoi(argv[2]);
  if (cache_slots <= 0)
  {
    fprintf(stderr, "Cache slot size must be a positive integer.\n");
    exit(EXIT_FAILURE);
  }

  // Choose replacement policy.
  if (argc >= 4)
  {
    if (strcmp(argv[3], "fifo") == 0)
      replacement_policy = REPLACEMENT_FIFO;
    else if (strcmp(argv[3], "lru") == 0)
      replacement_policy = REPLACEMENT_LRU;
    else if (strcmp(argv[3], "mru") == 0)
      replacement_policy = REPLACEMENT_MRU;
    else if (strcmp(argv[3], "random") == 0)
      replacement_policy = REPLACEMENT_RANDOM;
    else if (strcmp(argv[3], "lfu") == 0)
      replacement_policy = REPLACEMENT_LFU;
    else
      replacement_policy = REPLACEMENT_LRU;
  }
  else
  {
    // Default policy for this implementation: use LRU.
    replacement_policy = REPLACEMENT_LRU;
  }

  printf("page_size is %ld\n", page_size = sysconf(_SC_PAGESIZE));
  printf("Cache slots: %d, Replacement policy: %s\n\n", cache_slots,
         (replacement_policy == REPLACEMENT_LRU ? "LRU (stack)" : (replacement_policy == REPLACEMENT_FIFO ? "FIFO (queue)" : "Other")));

  // Allocate the sqrt region.
  setup_sqrt_region();

  // Reset counters.
  total_accesses = num_faults = evictions = 0;

  // Run selected workload.
  switch (workload)
  {
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

  // Clean up linked list if using LRU/FIFO.
  cache_node *curr = list_head;
  while (curr)
  {
    cache_node *next = curr->next;
    free(curr);
    curr = next;
  }

  return 0;
}
