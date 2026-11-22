//! Rust values backed by the shared ConFiGifY evaluator.
use std::collections::BTreeMap;
use std::ffi::{c_char, c_void, CStr, CString};
use std::fmt;
use std::ops::Index;
use std::path::Path;

#[derive(Debug, Clone, PartialEq)]
pub enum Value {
    Null,
    Bool(bool),
    Int(i64),
    Float(f64),
    String(String),
    List(Vec<Value>),
    Map(BTreeMap<String, Value>),
}

impl fmt::Display for Value {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Null => f.write_str("null"),
            Self::Bool(value) => write!(f, "{value}"),
            Self::Int(value) => write!(f, "{value}"),
            Self::Float(value) => write!(f, "{value}"),
            Self::String(value) => f.write_str(value),
            Self::List(items) => {
                f.write_str("[")?;
                for (i, item) in items.iter().enumerate() {
                    if i > 0 {
                        f.write_str(", ")?;
                    }
                    write!(f, "{item}")?;
                }
                f.write_str("]")
            }
            Self::Map(fields) => {
                f.write_str("{")?;
                for (i, (key, value)) in fields.iter().enumerate() {
                    if i > 0 {
                        f.write_str(", ")?;
                    }
                    write!(f, "{key:?}: {value}")?;
                }
                f.write_str("}")
            }
        }
    }
}

impl Index<&str> for Value {
    type Output = Value;
    fn index(&self, key: &str) -> &Self::Output {
        match self {
            Self::Map(map) => &map[key],
            _ => panic!("expected a map"),
        }
    }
}
impl Index<usize> for Value {
    type Output = Value;
    fn index(&self, i: usize) -> &Self::Output {
        match self {
            Self::List(items) => &items[i],
            _ => panic!("expected a list"),
        }
    }
}

#[repr(C)]
struct Span {
    data: *const u8,
    size: usize,
}
#[repr(C)]
struct Node {
    kind: i32,
    key: Span,
    integer: i64,
    number: f64,
    text: Span,
    count: usize,
    children: *const Node,
}
extern "C" {
    fn cfgy_parse(text: *const u8, size: usize, source: *const c_char) -> *mut c_void;
    fn cfgy_error(doc: *const c_void) -> *const c_char;
    fn cfgy_root(doc: *const c_void) -> *const Node;
    fn cfgy_free(doc: *mut c_void);
}
struct Document(*mut c_void);
impl Drop for Document {
    fn drop(&mut self) {
        unsafe { cfgy_free(self.0) }
    }
}

// Called only on borrowed views of a live native document.
unsafe fn string(span: &Span) -> Result<String, String> {
    if span.size == 0 {
        return Ok(String::new());
    }
    let bytes = std::slice::from_raw_parts(span.data, span.size);
    std::str::from_utf8(bytes)
        .map(str::to_owned)
        .map_err(|e| e.to_string())
}
unsafe fn convert(n: &Node) -> Result<Value, String> {
    let children = || {
        if n.count == 0 {
            &[]
        } else {
            std::slice::from_raw_parts(n.children, n.count)
        }
    };
    Ok(match n.kind {
        0 => Value::Null,
        1 => Value::Bool(n.integer != 0),
        2 => Value::Int(n.integer),
        3 => Value::Float(n.number),
        4 => Value::String(string(&n.text)?),
        5 => Value::List(
            children()
                .iter()
                .map(|c| convert(c))
                .collect::<Result<_, _>>()?,
        ),
        6 => {
            let mut map = BTreeMap::new();
            for c in children() {
                map.insert(string(&c.key)?, convert(c)?);
            }
            Value::Map(map)
        }
        _ => return Err("unknown cfgy type".into()),
    })
}
pub fn loads(text: &str) -> Result<Value, String> {
    loads_named(text, "<string>")
}
pub fn loads_named(text: &str, source: &str) -> Result<Value, String> {
    let source = CString::new(source).map_err(|e| e.to_string())?;
    unsafe {
        let doc = Document(cfgy_parse(text.as_ptr(), text.len(), source.as_ptr()));
        let err = cfgy_error(doc.0);
        if !err.is_null() {
            return Err(CStr::from_ptr(err).to_string_lossy().into_owned());
        }
        convert(&*cfgy_root(doc.0))
    }
}
pub fn load(path: impl AsRef<Path>) -> Result<Value, String> {
    let path = path.as_ref();
    let text = std::fs::read_to_string(path).map_err(|e| format!("{}: {e}", path.display()))?;
    loads_named(&text, &path.to_string_lossy())
}
