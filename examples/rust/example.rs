use cfgy::Value;

fn main() -> Result<(), String> {
    let path = std::env::args_os()
        .nth(1)
        .ok_or("expected a configuration path")?;
    let cfg = cfgy::load(path)?;
    if let (Value::Int(count), Value::Float(gain)) = (&cfg["c"]["myel1"], &cfg["myfloat2"]) {
        println!("{count}\n{gain}");
        Ok(())
    } else {
        Err("expected integer c.myel1 and float myfloat2".into())
    }
}
