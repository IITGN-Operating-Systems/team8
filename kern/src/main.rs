#![feature(alloc_error_handler)]
#![feature(decl_macro)]
#![feature(auto_traits)]
#![feature(negative_impls)]
#![cfg_attr(not(test), no_std)]
#![cfg_attr(not(test), no_main)]
#[warn(unused_attributes)]
#[warn(internal_features)]
#[warn(unused_imports)]

#[cfg(not(test))]
mod init;

extern crate alloc;

pub mod allocator;
pub mod console;
// pub mod fs;
pub mod mutex;
pub mod shell;

use console::kprintln;
use pi::uart::MiniUart;
use stack_vec::StackVec;
use core::{alloc::GlobalAlloc, fmt::Write};
// import stackVec present in /lib/stack-vec/src/lib.rs
use allocator::{Allocator, LocalAlloc};
// use fs::FileSystem;

#[cfg_attr(not(test), global_allocator)]
pub static ALLOCATOR: Allocator = Allocator::uninitialized();
// pub static FILESYSTEM: FileSystem = FileSystem::uninitialized();

fn kmain() -> ! {
    unsafe {
        ALLOCATOR.initialize();
        // FILESYSTEM.initialize();
    }

    // kprintln!("Welcome to cs330!");
    // shell::shell("> ");

    // shell::shell("$ ");
    // kprintln!("Shell exited. Press <Ctrl-A, X> to exit QEMU.");
    // Allocator::alloc(&mut self, layout: Layout) -> Result<*mut u8, alloc::alloc::AllocError> {
    let layout = alloc::alloc::Layout::from_size_align(50, 8).unwrap();
    let storage = unsafe { ALLOCATOR.alloc(layout) };

    if storage.is_null() {
        panic!("Allocation failed");
    }

    kprintln!("Allocated {} bytes at address {:p}", layout.size(), storage);

    let mut storage_slice = unsafe { core::slice::from_raw_parts_mut(storage, layout.size()) };
    
    let mut v = StackVec::new(storage_slice);
    for i in 0..50 {
        v.push(i).unwrap();
        // kprintln!("{:?}", v);
    }
    for i in 0..50 {
        v.pop().unwrap();
        // kprintln!("{:?}", v);
    }
    for i in 0..50 {
        v.push(i+50).unwrap();
        // kprintln!("{:?}", v);
    }   
    v.push(100).unwrap();
    kprintln!("{:?}", v);

    loop {
    }
}
