use core::alloc::Layout;
use core::fmt;
use core::ptr;

use crate::allocator::linked_list::LinkedList;
use crate::allocator::util::*; // Assume this defines `align_up`
use crate::allocator::LocalAlloc;

const NUM_BINS: usize = 30;

/// Map a requested allocation size to a bin index.
/// Each bin k handles allocations of size <= 2^(k+3).
fn map_to_bin(size: usize) -> Option<usize> {
    if size == 0 {
        return None;
    }
    let mut bin = 0;
    let mut block_size = 1 << 3; // 2^3 = 8 bytes for bin 0.
    // Increase bin until block_size is large enough
    while block_size < size {
        bin += 1;
        // If we run out of bins, then the request is too big.
        if bin >= NUM_BINS {
            return None;
        }
        block_size <<= 1; // multiply block_size by 2.
    }
    Some(bin)
}

/// A simple bin allocator.
/// Each bin holds free blocks of size 2^(k+3) for bin k.
pub struct Allocator {
    start: usize,      // beginning of managed region
    end: usize,        // end of managed region (exclusive)
    next: usize,       // next address to carve new blocks from
    bins: [LinkedList; NUM_BINS], // free lists for each bin
}

impl Allocator {
    /// Creates a new bin allocator that will allocate memory from the region
    /// starting at address `start` and ending at address `end`.
    pub fn new(start: usize, end: usize) -> Allocator {
        // Initialize bins using array::from_fn (requires a recent Rust version).
        let bins = core::array::from_fn(|_| LinkedList::new());
        Allocator {
            start,
            end,
            next: start,
            bins,
        }
    }
}

impl LocalAlloc for Allocator {
    /// Allocates memory according to the given layout.
    ///
    /// The block allocated will have a size equal to a power of two (2^(k+3))
    /// where k is determined by the requested layout.size(). If there is a free
    /// block in the corresponding bin, it is reused. Otherwise, a new block is
    /// carved out of the contiguous heap region.
    unsafe fn alloc(&mut self, layout: Layout) -> *mut u8 {
        // Map the requested allocation size to a bin.
        let bin = match map_to_bin(layout.size()) {
            Some(b) => b,
            None => return ptr::null_mut(),
        };
        // The size for this bin.
        let block_size = 1 << (bin + 3); // 2^(bin+3)
        // Check that the alignment of the layout is satisfied by the block.
        // (If the requested alignment is greater than the block size, or if the
        // block size isn’t a multiple of the requested alignment, we fail.)
        if layout.align() > block_size || block_size % layout.align() != 0 {
            return ptr::null_mut();
        }

        // If a previously freed block is available in the free list, use it.
        if let Some(block_addr) = self.bins[bin].pop() {
            return block_addr as *mut u8;
        }

        // Otherwise, carve a new block from the remaining heap region.
        let aligned_next = align_up(self.next, block_size);
        if aligned_next.checked_add(block_size).unwrap_or(self.end + 1) > self.end {
            return ptr::null_mut(); // Out of memory.
        }
        self.next = aligned_next + block_size;
        aligned_next as *mut u8
    }

    /// Deallocates the memory referenced by `ptr`.
    ///
    /// The block is simply pushed onto the free list corresponding to its size class.
    unsafe fn dealloc(&mut self, ptr: *mut u8, layout: Layout) {
        // Map the layout to the bin index.
        if let Some(bin) = map_to_bin(layout.size()) {
            self.bins[bin].push(ptr as *mut usize);
        }
        // Else: if the layout doesn’t correspond to a supported bin, do nothing.
    }
}

/// Provide a Debug implementation for Allocator.
impl fmt::Debug for Allocator {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Allocator")
            .field("start", &self.start)
            .field("end", &self.end)
            .field("next", &self.next)
            .finish()
    }
}
