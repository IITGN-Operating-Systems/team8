use shim::io;
use shim::path::{Path, PathBuf};
use core::str;
use pi::timer::current_time;
use crate::kprintln;
use core::arch::asm;
use crate::console::{kprint, CONSOLE};

use stack_vec::StackVec;

use pi::atags::Atags;

// use fat32::traits::FileSystem;
// use fat32::traits::{Dir, Entry};

use crate::ALLOCATOR;
// use crate::FILESYSTEM;

/// Error type for `Command` parse failures.
#[derive(Debug)]
enum Error {
    Empty,
    TooManyArgs,
}

/// A structure representing a single shell command.
struct Command<'a> {
    args: StackVec<'a, &'a str>,
}

impl<'a> Command<'a> {
    /// Parse a command from a string `s` using `buf` as storage for the
    /// arguments.
    ///
    /// # Errors
    ///
    /// If `s` contains no arguments, returns `Error::Empty`. If there are more
    /// arguments than `buf` can hold, returns `Error::TooManyArgs`.
    fn parse(s: &'a str, buf: &'a mut [&'a str]) -> Result<Command<'a>, Error> {
        let mut args = StackVec::new(buf);
        for arg in s.split(' ').filter(|a| !a.is_empty()) {
            args.push(arg).map_err(|_| Error::TooManyArgs)?;                //if size of args exceeds 64, return error
        }

        if args.is_empty() {
            return Err(Error::Empty);
        }

        Ok(Command { args })
    }

    /// Returns this command's path. This is equivalent to the first argument.
    fn path(&self) -> &str {
        self.args[0]
    }
}

/// Starts a shell using `prefix` as the prefix for each line. This function
/// returns if the `exit` command is called.
pub fn shell(prefix: &str) {
    let mut line = [0u8; 512];
    let mut buf = StackVec::new(&mut line);
    let mut exit = false;
    // kprintln!("Welcome to Rustberry Pi!");
    kprintln!("Team Let's Get Rusty Welcomes you to...");
    kprintln!(" ██████   ██████   ██████");
    kprintln!(" ██      ██    ██  ██    ");
    kprintln!(" ██████  ██    ██  ██████");
    kprintln!("     ██  ██    ██      ██");
    kprintln!(" ██████   ██████   ██████");
    while !exit{
        // kprint!("{} {}", pwd.to_str().unwrap(), prefix);
        kprint!("{}", prefix);
        read_command(&mut buf);
        exit = execute_command(&mut buf);
    }
}

fn read_command(mut buf: &mut StackVec<u8>) {
    loop {
        let input = CONSOLE.lock().read_byte();
        match input {
            8 | 127 => backspace(&mut buf),
            b'\r' | b'\n' => break,
            32..=126 => store_command(&mut buf, input),
            _ => ring_bell_sound(),
        }
    }
}

fn backspace(buf: &mut StackVec<u8>) {
    if buf.is_empty() {
        ring_bell_sound();
    } else {
        kprint!("\u{8} \u{8}");
        buf.pop();
    }
}

fn ring_bell_sound() {
    kprint!("\u{7}");
}

fn store_command(buf: &mut StackVec<u8>, input: u8) {
    match buf.push(input) {
        Ok(_) => CONSOLE.lock().write_byte(input),
        Err(_) => ring_bell_sound()
    }
}

fn execute_command(buf: &mut StackVec<u8>) -> bool {
    kprint!("\r\n");
    let mut binding = [""; 64];
    let cmd = Command::parse(str::from_utf8(buf.as_slice()).unwrap(), &mut binding);

    match cmd {
        Ok(cmd) => {
            match cmd.path() {
                "exit" => return true,
                "echo" => s_echo(&cmd.args[1..]),
                "time" => s_time(),
                "sleep" => {
                    if cmd.args.len() == 2 {
                        s_sleep(cmd.args[1]);
                    } else if cmd.args.len() == 1 {
                        kprint!("sleep: Argument Required!\r\nUsage: sleep <ms>\r\n");
                    } else {
                        kprint!("sleep: Too many arguments!\r\nUsage: sleep <ms>\r\n");
                    }
                }
                _ => kprint!("{}: command not found\r\n", cmd.path())   
            }
        },
        Err(Error::TooManyArgs) => kprint!("too many arguments\r\n"),
        _ => {}
    }
    buf.truncate(0);
    false
}

// -----------------------------------Implementing shell commands-----------------------------------

// fn s_ls(pwd: &mut PathBuf) {
//     unimplemented!();
// }

// fn s_pwd(pwd: &mut PathBuf) {
//     unimplemented!();
// }

fn s_echo(args: &[&str]) {
    for arg in args {
        kprint!("{} ", arg);
    }
    kprint!("\r\n");
}

// fn s_cd(pwd: &mut PathBuf, args: &[&str]) {
//     unimplemented!();
// }

// fn s_cat(pwd: &mut PathBuf, args: &[&str]) {
//     unimplemented!();
// }

fn s_sleep(arg: &str) {
    let ms = arg.parse::<u32>();
    match ms {
        Ok(ms) => {
            match sys_call_sleep(ms) {
                Ok(_) => kprint!("sleep: done\r\n"),
                Err(_) => kprint!("sleep: error\r\n")
            }
        },
        Err(_) => {
            kprint!("sleep: Invalid Argument\r\n");
            kprint!("usage: sleep <ms>\r\n");
        }
    }
}

fn s_time() {
    kprint!("System has been running for {:?}.\r\n", current_time());
}

fn sys_call_sleep(ms: u32) -> Result<u32, io::Error> {
    let start = current_time();
    let end = start + core::time::Duration::from_millis(ms as u64);
    while current_time() < end {}
    Ok(ms)
}
