import io.configify.Cfgy;
import java.nio.file.Path;

class Example {
    public static void main(String[] args) throws Exception {
        if (args.length != 1) throw new IllegalArgumentException("expected a configuration path");
        var cfg = Cfgy.load(Path.of(args[0]));
        System.out.println(Cfgy.lookup(cfg, "c", "myel1"));
        System.out.println(cfg.get("myfloat2"));
    }
}
