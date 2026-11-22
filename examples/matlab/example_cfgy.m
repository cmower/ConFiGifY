function example_cfgy(path)
    cfg = cfgy.load(path);
    fprintf('%d\n', cfg{'recording'}{'samples'});
    fprintf('%g\n', cfg{'recording'}{'interval_ms'});
end
