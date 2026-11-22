use std::env::args_os;

fn main() -> Result<(), String> {
    let path = args_os().nth(1).ok_or("expected a configuration path")?;
    let cfg = cfgy::load(path)?;
    println!("{}", cfg["recording"]["samples"]);
    println!("{}", cfg["recording"]["interval_ms"]);
    Ok(())
}
