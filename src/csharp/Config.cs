using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace Cfgy;

public static class Config
{
    [StructLayout(LayoutKind.Sequential)]
    private struct Span { public IntPtr Data; public UIntPtr Size; }
    [StructLayout(LayoutKind.Sequential)]
    private struct Node
    {
        public int Kind;
        public Span Key;
        public long Integer;
        public double Number;
        public Span Text;
        public UIntPtr Count;
        public IntPtr Children;
    }

    [DllImport("cfgy", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr cfgy_parse(byte[] text, UIntPtr size, [MarshalAs(UnmanagedType.LPUTF8Str)] string source);
    [DllImport("cfgy", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr cfgy_error(IntPtr doc);
    [DllImport("cfgy", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr cfgy_root(IntPtr doc);
    [DllImport("cfgy", CallingConvention = CallingConvention.Cdecl)]
    private static extern void cfgy_free(IntPtr doc);

    private static string Text(Span span) =>
        Marshal.PtrToStringUTF8(span.Data, checked((int)span.Size.ToUInt64())) ?? "";

    private static object? Unpack(Node node)
    {
        Node Child(int i) => Marshal.PtrToStructure<Node>(
            IntPtr.Add(node.Children, checked(i * Marshal.SizeOf<Node>())));
        switch (node.Kind)
        {
            case 0: return null;
            case 1: return node.Integer != 0;
            case 2: return node.Integer;
            case 3: return node.Number;
            case 4: return Text(node.Text);
            case 5:
                var list = new object?[checked((int)node.Count.ToUInt64())];
                for (int i = 0; i < list.Length; ++i) list[i] = Unpack(Child(i));
                return list;
            case 6:
                var map = new Dictionary<string, object?>();
                for (int i = 0; i < checked((int)node.Count.ToUInt64()); ++i)
                {
                    var child = Child(i);
                    map.Add(Text(child.Key), Unpack(child));
                }
                return map;
            default: throw new InvalidOperationException("unknown cfgy type");
        }
    }

    private static Dictionary<string, object?> Parse(byte[] bytes, string source)
    {
        if (source.Contains('\0')) throw new ArgumentException("NUL in source name");
        var doc = cfgy_parse(bytes, (UIntPtr)(uint)bytes.Length, source);
        try
        {
            var err = cfgy_error(doc);
            if (err != IntPtr.Zero) throw new FormatException(Marshal.PtrToStringUTF8(err));
            return (Dictionary<string, object?>)Unpack(Marshal.PtrToStructure<Node>(cfgy_root(doc)))!;
        }
        finally { cfgy_free(doc); }
    }

    public static Dictionary<string, object?> Loads(string text, string source = "<string>") =>
        Parse(Encoding.UTF8.GetBytes(text), source);
    public static Dictionary<string, object?> Load(string path) => Parse(File.ReadAllBytes(path), path);
}
