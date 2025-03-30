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

// extern crate alloc;

pub mod allocator;
pub mod console;
// pub mod fs;
pub mod mutex;
pub mod shell;

use console::kprintln;
use pi::uart::MiniUart;
use stack_vec::StackVec;
use core::fmt::Write;
// import stackVec present in /lib/stack-vec/src/lib.rs
use stack_vec::StackVec;
use allocator::Allocator;
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

    let mut storage = [0; 50];
    let mut v = StackVec::new(&mut storage);
    for i in 0..50 {
        v.push(i).unwrap();
        kprintln!("{:?}", v);
    }

    loop {
    }
}
