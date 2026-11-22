function example_cfgy(path)
    cfg = cfgy.load(path);
    c = cfg('c');
    disp(c('myel1'));
    disp(cfg('myfloat2'));
end
