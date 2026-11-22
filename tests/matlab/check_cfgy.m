function check_cfgy(path)
    cfg = cfgy.load(path);
    assert(isequal(cfg('wide'), int64(9007199254740992) + int64(1)));
    assert(strcmp(cfg('label'), 'café 雪 😀'));
    group = cfg('group');
    assert(isequal(group('a.b'), int64(7)));
    mixed = cfg('mixed');
    assert(isempty(mixed{1}) && islogical(mixed{2}) && iscell(mixed{7}));
    ab = cfg('absolute');
    assert(isequal(ab{1}, int64(3)) && isequal(ab{2}, 2.5));
    try
        cfgy.loads('x: sqrt(-1)');
        error('cfgy:Test', 'Expected a parse error');
    catch err
        assert(strcmp(err.identifier, 'cfgy:Parse'));
        assert(~isempty(strfind(err.message, '<string>:1:4:')));
    end
end
