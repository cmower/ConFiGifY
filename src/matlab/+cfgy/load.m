function config = load(path)
%LOAD Read a UTF-8 ConFiGifY file.
    [file, message] = fopen(path, 'rb');
    if file < 0
        error('cfgy:IO', '%s: %s', path, message);
    end
    closeFile = onCleanup(@() fclose(file));
    data = fread(file, Inf, '*uint8');
    [message, status] = ferror(file);
    if status ~= 0
        error('cfgy:IO', '%s: %s', path, message);
    end
    config = cfgy.loads(data, path);
end
