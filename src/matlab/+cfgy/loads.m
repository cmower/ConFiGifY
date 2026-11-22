function config = loads(text, source)
%LOADS Evaluate configuration text or UTF-8 bytes.
    if nargin < 2
        source = '<string>';
    end
    if ~isa(text, 'uint8')
        text = unicode2native(char(text), 'UTF-8');
    end
    [tree, message] = cfgy_native(text, unicode2native(char(source), 'UTF-8'));
    if ~isempty(message)
        error('cfgy:Parse', '%s', native2unicode(message, 'UTF-8'));
    end
    config = unpack(tree);
end

function value = unpack(node)
    if isstruct(node)
        value = cfgy.Config();
        for i = 1:numel(node.keys)
            key = char(native2unicode(node.keys{i}, 'UTF-8'));
            value(key) = unpack(node.values{i});
        end
    elseif iscell(node)
        value = cellfun(@unpack, node, 'UniformOutput', false);
    elseif isa(node, 'uint8')
        value = char(native2unicode(node, 'UTF-8'));
    else
        value = node;
    end
end
