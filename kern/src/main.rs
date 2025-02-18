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

use console::kprintln;
use mutex::Mutex;
use pi::uart::{self, MiniUart};

//static UART: Mutex<Option<MiniUart>> = Mutex::new(None);
use core::fmt::Write;

// import shell
use shell::shell;

fn kmain() -> ! {
    // Initialize the UART driver
    // UART.lock().replace(MiniUart::new());
    kprintln!("UART echo test: type something!");
    let mut u = uart::MiniUart::new();

    loop {
        // // Lock the UART to access it safely across cores
        // let mut uart = UART.lock();

        // // Check if a byte is available to read
        // if let Some(uart) = uart.as_mut() {
        //     if uart.has_byte() {
        //         // Read the byte and echo it back
        //         let byte = uart.read_byte();
        //         if byte == b'\r' {
        //             uart.write_byte(b'\n');
        //         }
        //         else if byte == 127 {
        //             uart.write_byte(8);
        //             uart.write_byte(b' ');
        //             uart.write_byte(8);
        //         }
        //         // kprintln!("kprint byte: {}", byte);
        //         uart.write_byte(byte);
        //     }
        // }
        let b = u.read_byte();
        u.write_byte(b);
        u.write_str("afdfds");
    }


    // Initialize the UART driver
    // UART.lock().replace(MiniUart::new());

    // // Start the shell
    // shell("$ ");

    // loop {}

}