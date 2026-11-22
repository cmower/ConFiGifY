package io.configify;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;

public final class Cfgy {
    static { System.loadLibrary("cfgy_jni"); }
    private Cfgy() {}

    public static Map<String, Object> load(Path path) throws IOException {
        return parse(Files.readAllBytes(path), sourceBytes(path.toString()));
    }
    public static Map<String, Object> loads(String text) { return loads(text, "<string>"); }
    public static Map<String, Object> loads(String text, String source) {
        return parse(text.getBytes(StandardCharsets.UTF_8), sourceBytes(source));
    }

    /** Follows literal map keys. An empty path selects the root.
     * @throws NoSuchElementException if a key is missing or its parent is not a map
     */
    public static Object lookup(Map<String, ?> cfg, String... keys) {
        Object value = cfg;
        for (String key : keys) {
            if (!(value instanceof Map<?, ?>)) {
                throw new NoSuchElementException("Expected a map before key '" + key + "'");
            }
            Map<?, ?> map = (Map<?, ?>) value;
            if (!map.containsKey(key)) throw new NoSuchElementException("Missing key '" + key + "'");
            value = map.get(key);
        }
        return value;
    }

    private static byte[] sourceBytes(String source) {
        if (source.indexOf('\0') >= 0) throw new IllegalArgumentException("NUL in source name");
        return (source + '\0').getBytes(StandardCharsets.UTF_8);
    }
    private static native Map<String, Object> parse(byte[] text, byte[] source);

    // Called by JNI. Standard UTF-8 handles NUL and supplementary code points.
    private static Object make(int kind, long integer, double number, byte[] text) {
        switch (kind) {
            case 0: return null;
            case 1: return integer != 0;
            case 2: return integer;
            case 3: return number;
            case 4: return new String(text, StandardCharsets.UTF_8);
            case 5: return new ArrayList<Object>();
            case 6: return new LinkedHashMap<String, Object>();
            default: throw new IllegalStateException("unknown cfgy type");
        }
    }
    @SuppressWarnings("unchecked")
    private static void add(Object parent, byte[] key, Object value) {
        if (key == null) ((List<Object>)parent).add(value);
        else ((Map<String, Object>)parent).put(new String(key, StandardCharsets.UTF_8), value);
    }
    private static void fail(byte[] message) {
        throw new IllegalArgumentException(new String(message, StandardCharsets.UTF_8));
    }
}
