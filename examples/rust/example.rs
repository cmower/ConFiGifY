use std::env::args_os;

fn main() -> Result<(), String> {
    let path = args_os().nth(1).ok_or("expected a configuration path")?;
    let cfg = cfgy::load(path)?;
    println!("{}", cfg["c"]["myel1"]);
    println!("{}", cfg["myfloat2"]);
    Ok(())
}
