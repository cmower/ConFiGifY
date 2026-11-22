import io.configify.Cfgy;
import java.nio.file.Paths;
import java.util.List;
import java.util.Map;

class Check {
    public static void main(String[] args) throws Exception {
        Map<String, Object> cfg = Cfgy.load(Paths.get(args[0]));
        List<?> mixed = (List<?>)cfg.get("mixed");
        if (!cfg.get("wide").equals(9007199254740993L) || !cfg.get("label").equals("café 雪 😀") ||
            !((Map<?, ?>)cfg.get("group")).get("a.b").equals(7L) || mixed.get(0) != null ||
            !mixed.get(1).equals(true) || !((List<?>)mixed.get(6)).isEmpty() ||
            !((List<?>)cfg.get("absolute")).get(1).equals(2.5)) throw new AssertionError(cfg);
        try { Cfgy.loads("x: sqrt(-1)"); throw new AssertionError("expected error"); }
        catch (IllegalArgumentException e) { if (!e.getMessage().contains("<string>:1:4:")) throw e; }
    }
}
