use core::panic::PanicInfo;

// may need kprintln!() instead of println!() -> Let's see
use crate::console::kprintln;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    kprintln!("            (");
    kprintln!("       (      )     )");
    kprintln!("         )   (    (");
    kprintln!("        (          `");
    kprintln!("    .-\"\"^\"\"\"^\"\"^\"\"\"^\"\"-.");
    kprintln!("  (//\\\\//\\\\//\\\\//\\\\//\\\\//)");
    kprintln!("   ~\\^^^^^^^^^^^^^^^^^^/~");
    kprintln!("     `================`");
    kprintln!();
    kprintln!("    The pi is overdone.");
    kprintln!();
    kprintln!("---------- PANIC ----------");
    kprintln!("");
    if let Some(location) = _info.location() {
        kprintln!("FILE: {}", location.file());
        kprintln!("LINE: {}", location.line());
        kprintln!("COLUMN: {}", location.column());
    } 
    kprintln!("");
    kprintln!("{}", _info.message());
    loop {}
}
