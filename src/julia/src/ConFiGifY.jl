module ConFiGifY

export load, loads
const lib = "libcfgy"

struct Span
    data::Ptr{UInt8}
    size::Csize_t
end
struct Node
    kind::Int32
    key::Span
    integer::Int64
    number::Float64
    text::Span
    count::Csize_t
    children::Ptr{Cvoid}
end
text(s::Span) = s.size == 0 ? "" : unsafe_string(s.data, Int(s.size))

function unpack(n::Node)
    n.kind == 0 && return nothing
    n.kind == 1 && return n.integer != 0
    n.kind == 2 && return n.integer
    n.kind == 3 && return n.number
    n.kind == 4 && return text(n.text)
    children = Ptr{Node}(n.children)
    if n.kind == 5
        return Any[unpack(unsafe_load(children, i)) for i in 1:Int(n.count)]
    elseif n.kind == 6
        map = Dict{String,Any}()
        for i in 1:Int(n.count)
            child = unsafe_load(children, i)
            map[text(child.key)] = unpack(child)
        end
        return map
    end
    error("unknown cfgy type")
end

function loads(input::AbstractString; source::AbstractString="<string>")
    data = String(input)
    doc = ccall((:cfgy_parse, lib), Ptr{Cvoid}, (Ptr{UInt8}, Csize_t, Cstring),
                data, sizeof(data), String(source))
    try
        err = ccall((:cfgy_error, lib), Cstring, (Ptr{Cvoid},), doc)
        err == C_NULL || throw(ArgumentError(unsafe_string(err)))
        root = ccall((:cfgy_root, lib), Ptr{Node}, (Ptr{Cvoid},), doc)
        return unpack(unsafe_load(root))
    finally
        ccall((:cfgy_free, lib), Cvoid, (Ptr{Cvoid},), doc)
    end
end
load(path::AbstractString) = loads(read(path, String); source=path)
end
