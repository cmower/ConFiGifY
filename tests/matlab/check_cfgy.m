function check_cfgy(path)
    cfg = cfgy.load(path);
    assert(isa(cfg, 'containers.Map') && isKey(cfg, 'group'));
    assert(isequal(cfg{'group'}{'a.b'}, int64(7)));
    assert(strcmp(cfg{'label'}, 'café 雪 😀'));
    assert(isequal(cfg{'absolute'}{2}, 2.5));
    assert(isempty(cfg{'mixed'}{1}));
    assert(isequal(cfg('wide'), int64(9007199254740992) + int64(1)));
    assert(strcmp(cfg('label'), 'café 雪 😀'));
    group = cfg('group');
    assert(isequal(group('a.b'), int64(7)));
    mixed = cfg('mixed');
    assert(isempty(mixed{1}) && islogical(mixed{2}) && iscell(mixed{7}));
    ab = cfg('absolute');
    assert(isequal(ab{1}, int64(3)) && isequal(ab{2}, 2.5));
    nested = cfgy.loads(sprintf('items:\n  - Count: 5\n'));
    assert(isequal(nested{'items'}{1}{'Count'}, int64(5)));
    try
        missing = cfg{'group'}{'missing'};
        error('cfgy:Test', 'Expected a missing key error');
    catch err
        assert(~strcmp(err.identifier, 'cfgy:Test'));
    end
    try
        extra = cfg{'group', 'a.b'};
        error('cfgy:Test', 'Expected separate braces for each map key');
    catch err
        assert(strcmp(err.identifier, 'cfgy:Index'));
    end
    try
        cfgy.loads('x: sqrt(-1)');
        error('cfgy:Test', 'Expected a parse error');
    catch err
        assert(strcmp(err.identifier, 'cfgy:Parse'));
        assert(~isempty(strfind(err.message, '<string>:1:4:')));
    end
end
