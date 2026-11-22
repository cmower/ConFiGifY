import io.configify.Cfgy;
import java.nio.file.Paths;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;

class Check {
    public static void main(String[] args) throws Exception {
        Map<String, Object> cfg = Cfgy.load(Paths.get(args[0]));
        List<?> mixed = (List<?>)cfg.get("mixed");
        if (!cfg.get("wide").equals(9007199254740993L) || !cfg.get("label").equals("café 雪 😀") ||
            !Cfgy.lookup(cfg, "group", "a.b").equals(7L) || mixed.get(0) != null ||
            !mixed.get(1).equals(true) || !((List<?>)mixed.get(6)).isEmpty() ||
            !((List<?>)cfg.get("absolute")).get(1).equals(2.5)) throw new AssertionError(cfg);
        var nested = Cfgy.loads("a:\n  b:\n    c: null");
        if (Cfgy.lookup(nested) != nested || Cfgy.lookup(nested, "a", "b", "c") != null)
            throw new AssertionError(nested);
        expectMissing(cfg, "group", "missing");
        expectMissing(cfg, "wide", "child");
        expectMissing(nested, "a", "b", "c", "child");
        try { Cfgy.loads("x: sqrt(-1)"); throw new AssertionError("expected error"); }
        catch (IllegalArgumentException e) { if (!e.getMessage().contains("<string>:1:4:")) throw e; }
    }

    private static void expectMissing(Map<String, Object> cfg, String... keys) {
        try { Cfgy.lookup(cfg, keys); }
        catch (NoSuchElementException expected) { return; }
        throw new AssertionError("Expected lookup to fail for " + String.join(", ", keys));
    }
}
