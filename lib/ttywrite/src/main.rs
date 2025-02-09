mod parsers;

use serial;
use structopt;
use structopt_derive::StructOpt;
use xmodem::Xmodem;

use std::path::PathBuf;
use std::time::Duration;

use structopt::StructOpt;
use serial::core::{CharSize, BaudRate, StopBits, FlowControl, SerialDevice, SerialPortSettings};

use parsers::{parse_width, parse_stop_bits, parse_flow_control, parse_baud_rate};

#[derive(StructOpt, Debug)]
#[structopt(about = "Write to TTY using the XMODEM protocol by default.")]
struct Opt {
    #[structopt(short = "i", help = "Input fil  e (defaults to stdin if not set)", parse(from_os_str))]
    input: Option<PathBuf>,

    #[structopt(short = "b", long = "baud", parse(try_from_str = "parse_baud_rate"),
                help = "Set baud rate", default_value = "115200")]
    baud_rate: BaudRate,

    #[structopt(short = "t", long = "timeout", parse(try_from_str),
                help = "Set timeout in seconds", default_value = "10")]
    timeout: u64,

    #[structopt(short = "w", long = "width", parse(try_from_str = "parse_width"),
                help = "Set data character width in bits", default_value = "8")]
    char_width: CharSize,

    #[structopt(help = "Path to TTY device", parse(from_os_str))]
    tty_path: PathBuf,

    #[structopt(short = "f", long = "flow-control", parse(try_from_str = "parse_flow_control"),
                help = "Enable flow control ('hardware' or 'software')", default_value = "none")]
    flow_control: FlowControl,

    #[structopt(short = "s", long = "stop-bits", parse(try_from_str = "parse_stop_bits"),
                help = "Set number of stop bits", default_value = "1")]
    stop_bits: StopBits,

    #[structopt(short = "r", long = "raw", help = "Disable XMODEM")]
    raw: bool,
}

fn main() {
    use std::fs::File;
    use std::io::{self, BufReader};

    let opt = Opt::from_args();
    let mut port = serial::open(&opt.tty_path).expect("path points to invalid TTY");

    // FIXME: Implement the `ttywrite` utility.
    
    let mut settings = port.read_settings().expect("Couldn't read Settings");
    settings.set_baud_rate(opt.baud_rate).expect("Couldn't set baud_rate");
    settings.set_char_size(opt.char_width);
    settings.set_stop_bits(opt.stop_bits);
    settings.set_flow_control(opt.flow_control);
    port.write_settings(&settings).expect("Couldn't write Settings");
    port.set_timeout(Duration::from_secs(opt.timeout)).expect("Invalid Timeout");


    let mut input_data: Box<dyn io::Read> = match opt.input { // can't implement it without using Box<dyn io::Read> as both File and Stdin are different types and match should return same type
        Some(path) => {
            Box::new(BufReader::new(File::open(path).expect("Couldn't open file")))
        },
        None => {
            Box::new(BufReader::new(io::stdin()))
        }
    };

    if opt.raw == true {    
        io::copy(&mut input_data, &mut port).expect("Couldn't copy data");
    } else {
        match Xmodem::transmit_with_progress(input_data, port, progress_fn) {
            Ok(_) => println!("Transmission successful"),
            Err(e) => panic!("Error: {:?}", e),
        }
    }

}

fn progress_fn(progress: xmodem::Progress) {
    println!("Progress: {:?}", progress);
}


/*
Question: What happens when a flag’s input is invalid?

StructOpt rejects invalid flag values because custom parsing functions (e.g., parse_flow_control) return a Result. For example, if -f idk is provided, parse_flow_control returns an Err, prompting StructOpt to display an error and exit. These parsers validate inputs by returning Ok only for valid values, ensuring invalid inputs are rejected early in argument parsing.

Question: Why does the test.sh script always set -r? (bad-tests)

The test.sh script uses -r (raw mode) because XMODEM requires a responsive receiver for protocol handshakes (e.g., ACK/NAK). Testing with XMODEM would need a mock receiver that implements the protocol, which the script's PTY setup likely lacks. Raw mode bypasses this by transmitting data directly without protocol checks, simplifying validation of basic I/O functionality without complex two-way communication emulation.
*/