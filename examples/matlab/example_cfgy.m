function example_cfgy(path)
    cfg = cfgy.load(path);
    disp(cfg{'c'}{'myel1'});
    disp(cfg{'myfloat2'});
end
