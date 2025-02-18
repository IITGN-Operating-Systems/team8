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

pub mod console;
pub mod mutex;
pub mod shell;

use console::{kprint,kprintln};
use pi::uart::MiniUart;
use core::fmt::Write;

// import shell
// use shell::shell;

fn kmain() -> ! {
    kprintln!("Welcome to Rustberry Pi!");
    let mut u = MiniUart::new();

    loop {
        let b = u.read_byte();
        u.write_byte(b);
        // u.write_str("hi\n");
    }
}