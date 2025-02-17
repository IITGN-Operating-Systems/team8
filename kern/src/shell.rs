use stack_vec::StackVec;
use std::path::PathBuf;
use crate::console::{kprint, kprintln, CONSOLE};

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
            args.push(arg).map_err(|_| Error::TooManyArgs)?;
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
    let mut pwd = PathBuf::from("/");
    let mut exit = false;
    while !exit{
        kprint!("{} {}", pwd.to_str().unwrap(), prefix);
        read_command(&mut buf);
        exit = execute_command(&mut buf, &mut pwd);
    }

}

fn read_command(mut buf: &mut StackVec<u8>) {
    loop {
        let input = CONSOLE.lock().read_byte();
        match input {
            8 | 127 => backspace(&mut buf),
            b'\r' | b'\n' => break,
            32..=126 => store_command(&mut buf, input),
            _ => ring_bell(),
        }
    }
}

fn backspace(buf: &mut StackVec<u8>) {
    if buf.is_empty() {
        ring_bell();
    } else {
        kprint!("\u{8} \u{8}");
        buf.pop();
    }
}

fn ring_bell() {
    kprint!("\u{7}");
}

fn store_command(buf: &mut StackVec<u8>, input: u8) {
    match buf.push(input) {
        Ok(_) => CONSOLE.lock().write_byte(input),
        Err(_) => ring_bell()
    }
}

fn execute_command(buf: &mut StackVec<u8>, pwd: &mut PathBuf) -> bool {
    let cmd = Command::parse(str::from_utf8(buf.as_slice()).unwrap(), &mut [""; 64]);

    match cmd {
        Ok(cmd) => {
            match cmd.path() {
                "exit" => return true,
                "ls" => s_ls(pwd),
                "pwd" => s_pwd(pwd),
                "echo" => s_echo(&cmd.args[1..]),
                "cd" => s_cd(pwd, &cmd.args[1..]),
                "cat" => s_cat(pwd, &cmd.args[1..]),
                "sleep" => s_sleep(cmd.args[1]),
                "time" => s_time(),
                _ => kprint!("command not found\r\n")
            }
        },
        Err(Error::TooManyArgs) => kprint!("too many arguments\r\n"),
        _ => {}

    }
    buf.truncate(0);
    false
}

// -----------------------------------Implementing shell commands-----------------------------------

fn s_ls(pwd: &mut PathBuf) {
    let dir: Option<Dir> = FILE_SYSTEM.get().open_dir(pwd.as_path()).ok();
    let entries = dir.unwrap().entries().unwrap();
    for d in entries {
        if d.is_file() {
            kprint!("-");
        } else {
            kprint!("d");
        }
        kprint!("\t{}\r\n", d.name());
    }
}

fn s_pwd(pwd: &mut PathBuf) {
    kprint!("{}\r\n", pwd.to_str().unwrap());
}

fn s_echo(args: &[&str]) {
    for arg in args {
        kprint!("{} ", arg);
    }
    kprint!("\r\n");
}

fn s_cd(pwd: &mut PathBuf, args: &[&str]) {
    let target = match args.len() {
        0 => "/",
        _ => args[0],
    };

    match target {
        ".." => { pwd.pop(); }
        "." => {}
        _ => {
            let mut new_dir = pwd.clone();
            new_dir.push(target);

            let dir = FILE_SYSTEM.get().open_dir(new_dir.as_path());
            match dir {
                Ok(_) => {
                    pwd.push(target);
                }
                Err(err) => kprint!("{}\r\n",err)
            }
        }
    }
}

fn s_cat(pwd: &mut PathBuf, args: &[&str]) {
    for filename in args {
        let mut file = pwd.clone();
        file.push(filename);
        match FILE_SYSTEM.get().open_file(file.as_path()) {
            Ok(mut f) => {
                let mut offset = 0;
                loop {
                    let _ = f.seek(SeekFrom::Current(offset));
                    let mut buf = [0u8; 512];
                    let bytes_read = f.read(&mut buf).unwrap() as i64;
                    if bytes_read == 0 {
                        break;
                    } else {
                        offset += bytes_read;
                        kprint!("{}", String::from_utf8_lossy(&buf));
                    }
                }
            }
            Err(err) => kprint!("{}\r\n",err)
        }
    }
}

fn s_sleep(arg: &str) {
    let ms = u32::from_str(arg).unwrap();
    let actual = sys_call_sleep(ms).unwrap();
    kprint!("elapsed {} ms\r\n", actual);
}

fn s_time() {
    kprint!("{}\r\n", current_time());
}

fn sys_call_sleep(ms: u32) -> Result<u32, std::io::Error> {
    let error: u64;
    let result: u64;
    unsafe {
        asm!("mov x0, $2
              svc 1
              mov $0, x0
              mov $1, x7"
              : "=r"(result), "=r"(error)
              : "r"(ms)
              : "x0", "x7")
    }

    if error != 0 {
        Err(std::io::Error::new(std::io::ErrorKind::Other, ""))
    } else {
        Ok(result as u32)
    }
}
