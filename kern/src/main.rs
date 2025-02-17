#![feature(alloc_error_handler)]
#![feature(decl_macro)]
#![feature(auto_traits)]
#![feature(negative_impls)]
#![cfg_attr(not(test), no_std)]
#![cfg_attr(not(test), no_main)]

#[cfg(not(test))]
mod init;

pub mod console;
pub mod mutex;
pub mod shell;

use console::kprintln;

// FIXME: You need to add dependencies here to
// test your drivers (Phase 2). Add them as needed.

use pi::uart::MiniUart;

fn kmain() -> ! {
    // FIXME: Start the shell.

    let mut uart = MiniUart::new();

    loop {
        let byte = uart.read_byte(); // Read one byte from UART
        uart.write_byte(byte); // Echo the byte back
    }
}
