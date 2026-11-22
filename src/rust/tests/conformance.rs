use cfgy::{load, loads, Value::*};

#[test]
fn shared_fixture() {
    let cfg = load(concat!(
        env!("CARGO_MANIFEST_DIR"),
        "/../../tests/conformance.cfgy"
    ))
    .unwrap();
    assert_eq!(cfg["wide"], Int(9007199254740993));
    assert_eq!(cfg["label"], String("café 雪 😀".into()));
    assert_eq!(cfg["group"]["a.b"], Int(7));
    assert_eq!(cfg["absolute"][1], Float(2.5));
    assert_eq!(cfg["mixed"][0], Null);
    assert_eq!(cfg["mixed"][1], Bool(true));
    assert_eq!(cfg["mixed"][6], List(vec![]));
    assert!(loads("x: sqrt(-1)").unwrap_err().contains("<string>:1:4:"));
}

#[test]
fn display_values() {
    let cfg = loads("label: 'café'\nvalues: [null, true, 7, 2.5, []]").unwrap();
    assert_eq!(cfg["label"].to_string(), "café");
    assert_eq!(
        cfg.to_string(),
        r#"{"label": café, "values": [null, true, 7, 2.5, []]}"#
    );
}
