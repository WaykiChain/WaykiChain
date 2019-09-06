import re

from regexes import (
    CALL_REGEX, CALL_INDIRECT_REGEX,
    ELEM_REGEX, EXPORT_REGEX,
    FUNC_REGEX, GLOBAL_REGEX,
    IMPORT_REGEX, START_REGEX, TYPE_REGEX
)
from wasm import WASM

class TestWASM(WASM):
    def __init__(self):
        super(TestWASM, self).__init__()

    def shift_types(self, max_type):
        type_map = {}
        new_types = []
        for t in self.types:
            match = re.search(TYPE_REGEX, t)

            type_num = match.group(2)
            new_type_num = int(type_num) + int(max_type) + 1
            type_map[type_num] = new_type_num

            new_type = re.sub(TYPE_REGEX, lambda x: f'{x.group(1)}{new_type_num}{x.group(3)}', t)
            new_types.append(new_type)

        self.types = new_types
        return type_map

    def shift_imports(self, type_map, max_import):
        new_imports = []
        for i in self.imports:
            m = re.search(IMPORT_REGEX, i)

            type_num = m.group(8)
            new_type_num = type_map[type_num]

            func_num = m.group(6)
            new_func_num = int(max_import) + 1
            self.function_symbol_map[func_num] = new_func_num

            sub_func = lambda x: f'{m.group(1)}{m.group(2)}{m.group(3)}{m.group(4)}{m.group(5)}{new_func_num}{m.group(7)}{new_type_num}{m.group(9)}'
            new_import = re.sub(IMPORT_REGEX, sub_func, i)
            new_imports.append(new_import)

            max_import = int(max_import) + 1

        self.imports = new_imports
        return max_import

    def shift_funcs(self, type_map, max_func_num):
        def get_type_num(f):
            return re.search(FUNC_REGEX, f).group(4)

        def get_func_num(f):
            return re.search(FUNC_REGEX, f).group(2)

        def inject_new_type_num(f, new_type_num):
            sub_func = lambda x: f'{x.group(1)}{x.group(2)}{x.group(3)}{new_type_num}{x.group(5)}'
            return re.sub(FUNC_REGEX, sub_func, f)

        def inject_new_func_num(f, new_func_num):
            sub_func = lambda x: f'{x.group(1)}{new_func_num}{x.group(3)}{x.group(4)}{x.group(5)}'
            return re.sub(FUNC_REGEX, sub_func, f)

        new_funcs = []
        for f in self.funcs:
            type_num = get_type_num(f)
            new_type_num = type_map[type_num]

            func_num = get_func_num(f)
            new_func_num = int(max_func_num) + 1

            self.function_symbol_map[func_num] = new_func_num

            max_func_num = new_func_num

            new_func = inject_new_func_num(f, new_func_num)
            new_func = inject_new_type_num(new_func, new_type_num)

            new_funcs.append(new_func)

        self.funcs = new_funcs

        return max_func_num

    def shift_calls(self, type_map):
        new_funcs = []
        for f in self.funcs:
            func = ''
            for l in f.split('\n'):
                if re.search(CALL_INDIRECT_REGEX, l):
                    func += self.shift_call_indirect(l, type_map) + '\n'
                elif re.search(CALL_REGEX, l):
                    func += self.shift_call(l) + '\n'
                else:
                    func += l + '\n'

            new_funcs.append(func)

        self.funcs = new_funcs

    def shift_call(self, line):
        match = re.search(CALL_REGEX, line)
        func_num = match.group(2)

        new_num = self.function_symbol_map[func_num]
        new_call = re.sub(CALL_REGEX, lambda x: f'{x.group(1)}{new_num}{x.group(3)}', line)

        return new_call

    def shift_call_indirect(self, line, type_map):
        match = re.search(CALL_INDIRECT_REGEX, line)
        type_num = match.group(2)
        new_num = type_map[type_num]

        new_call = re.sub(CALL_INDIRECT_REGEX, lambda x: f'{x.group(1)}{new_num}{x.group(3)}', line)

        return new_call

    def shift_start(self):
        if self.start:
            match = re.search(START_REGEX, self.start)
            func_num = match.group(2)

            new_num = self.function_symbol_map[func_num]
            new_start = re.sub(START_REGEX, lambda x: f'{x.group(1)}{new_num}{x.group(3)}', self.start)

            self.start = new_start


    def shift_exports(self):
        def get_func_num(f):
            return re.search(EXPORT_REGEX, f).group(4)

        def inject_new_func_num(f, new_func_num):
            return re.sub(EXPORT_REGEX, lambda x: f'{x.group(1)}{x.group(2)}{x.group(3)}{new_func_num}{x.group(5)}', f)

        def normalize(val):
            ret_val = '_'
            for i in range(0, len(val)):
                ret_val += '_' if val[i] == '-' or val[i] == '.' else val[i]

            return ret_val

        new_exports = []
        exports_map = {}
        for e in self.exports:
            if e.find('(func ') > -1:
                func_num = get_func_num(e)
                new_func_num = self.function_symbol_map[func_num]

                new_export = inject_new_func_num(e, new_func_num)

                new_exports.append(new_export)
            else:
                new_exports.append(e)

            exports_map[normalize(re.search(EXPORT_REGEX, e).group(2))] = new_func_num
        self.exports = new_exports
        return exports_map

    def shift_elems(self):
        new_elems = []
        for e in self.elems:
            match = re.search(ELEM_REGEX, e)
            numbers = match.group(2)
            new_numbers = []
            for n in numbers.split():
                shifted_num = self.function_symbol_map[n]
                new_numbers.append(str(shifted_num))

            new_numbers_str = ' '.join(new_numbers)
            sub_func = lambda x: f'{x.group(1)}{new_numbers_str}{x.group(3)}'
            new_elems.append(re.sub(ELEM_REGEX, sub_func, e))

        self.elems = new_elems

    def get_max_global(self):
        max_global_var = -1
        for g in self.global_vars:
            match = re.search(GLOBAL_REGEX, g)
            max_global_var = match.group(2)

        return max_global_var
