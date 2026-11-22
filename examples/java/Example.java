import io.configify.Cfgy;
import java.nio.file.Paths;
import java.util.Map;

class Example {
    public static void main(String[] args) throws Exception {
        if (args.length != 1) throw new IllegalArgumentException("expected a configuration path");
        Map<String, Object> cfg = Cfgy.load(Paths.get(args[0]));
        System.out.println(((Map<?, ?>)cfg.get("c")).get("myel1"));
        System.out.println(cfg.get("myfloat2"));
    }
}
