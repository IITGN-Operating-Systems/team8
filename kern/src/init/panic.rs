use core::panic::PanicInfo;

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
        println!();
        println!("{}", _info.message().unwrap_or(&"<no message>"));
    } else {
        println!("Panic: {}", _info.message().unwrap_or(&"<no message>"));
    }
    loop {}
}
