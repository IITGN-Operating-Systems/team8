use core::panic::PanicInfo;

// may need kprintln!() instead of println!() -> Let's see
// use crate::console::kprintln;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    println!("            (");
    println!("       (      )     )");
    println!("         )   (    (");
    println!("        (          `");
    println!("    .-\"\"^\"\"\"^\"\"^\"\"\"^\"\"-.");
    println!("  (//\\\\//\\\\//\\\\//\\\\//\\\\//)");
    println!("   ~\\^^^^^^^^^^^^^^^^^^/~");
    println!("     `================`");
    println!();
    println!("    The pi is overdone.");
    println!();
    println!("---------- PANIC ----------");
    println!();
    if let Some(location) = _info.location() {
        println!("FILE: {}", location.file());
        println!("LINE: {}", location.line());
        println!("COLUMN: {}", location.column());
    } 
    println!();
    if let Some(message) = _info.message() {
        println!("{:?}", message);
    } else {
        println!("MESSAGE: None");
    }
    loop {}
}
