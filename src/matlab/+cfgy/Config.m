classdef Config < containers.Map
%CONFIG A configuration map with chained brace lookup.
    methods
        function obj = Config()
            obj@containers.Map('KeyType', 'char', 'ValueType', 'any');
        end

        function varargout = subsref(obj, indices)
            if ~strcmp(indices(1).type, '{}')
                [varargout{1:nargout}] = subsref@containers.Map(obj, indices);
                return
            end

            key = indices(1);
            if numel(key.subs) ~= 1
                error('cfgy:Index', 'Use one map key per pair of braces');
            end
            key.type = '()';
            value = subsref@containers.Map(obj, key);
            if numel(indices) == 1
                varargout = {value};
            else
                [varargout{1:nargout}] = subsref(value, indices(2:end));
            end
        end
    end
end
