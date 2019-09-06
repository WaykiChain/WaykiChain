import re

from regexes import (
    CALL_REGEX, CALL_INDIRECT_REGEX,
    EXPORT_REGEX, FUNC_REGEX,
    GET_SET_GLOBAL_REGEX, GLOBAL_REGEX,
    IMPORT_REGEX
)
from wasm import WASM

class GeneratedWASM(WASM):
    def __init__(self):
        super(GeneratedWASM, self).__init__()
        self.base_funcs = []
        self.end_funcs = []
        self.imports_map = {}
        self.num_imports_base_functions = 0

    def shift_base_funcs(self, max_func):
        max_import = self.get_max_import()

        new_starting_index = 0
        self.num_imports_base_functions = max_import
        for f in self.funcs:
            for l in f.split('\n'):
                match = re.search(FUNC_REGEX, l)
                if match:
                    func_num = int(match.group(2))
                    # The "base functions" are the 3 functions that always follow the imports.
                    if func_num > int(max_import) and func_num <= int(max_import) + 3:
                        new_func_num, new_func = self.shift_func(f, max_func)
                        self.base_funcs.append(new_func)
                        self.function_symbol_map[match.group(2)] = new_func_num

                        new_starting_index += 1
                        max_func = int(max_func) + 1
                        self.num_imports_base_functions = int(self.num_imports_base_functions) + 1

        self.funcs = self.funcs[new_starting_index:]
        return max_func

    def shift_func(self, func, max_function_num):
        new_func_num = int(max_function_num) + 1
        sub_func = lambda x: f'{x.group(1)}{new_func_num}{x.group(3)}{x.group(4)}{x.group(5)}'
        new_func = re.sub(FUNC_REGEX, sub_func, func)

        return (new_func_num, new_func)

    def shift_funcs(self, num_to_name_map, max_func_num):
        end_funcs = []
        for f in self.funcs:
            match = re.search(FUNC_REGEX, f)
            func_num = match.group(2)
            if int(func_num) in num_to_name_map:
                continue

            max_func_num, new_func = self.shift_func(f, max_func_num)
            self.function_symbol_map[func_num] = max_func_num
            end_funcs.append(new_func)

        self.end_funcs = end_funcs
        return max_func_num

    def shift_calls(self, num_to_name_map, export_map):
        new_end_funcs = []
        for e in self.end_funcs:
            func = ''
            for l in e.split('\n'):
                if re.search(CALL_INDIRECT_REGEX, l):
                    pass
                elif re.search(CALL_REGEX, l):
                    num = re.search(CALL_REGEX, l).group(2)
                    if int(num) in num_to_name_map:
                        name = num_to_name_map[int(num)]
                        new_num = export_map[name]
                        new_call = re.sub(CALL_REGEX, lambda x: f'{x.group(1)}{new_num}{x.group(3)}', l)
                        func += new_call + '\n'
                    elif num in self.function_symbol_map:
                        new_num = self.function_symbol_map[num]
                        new_call = re.sub(CALL_REGEX, lambda x: f'{x.group(1)}{new_num}{x.group(3)}', l)
                        func += new_call + '\n'
                    elif num in self.imports_map:
                        # We're calling an import so we don't need to do anything
                        func += l
                    else:
                        print('-----', e)
                        print('Error attempting to shift calls in compiled wasm')
                        raise Exception('Error attempting to shift calls in compiled wasm.')
                else:
                    func += l + '\n'

            new_end_funcs.append(func)

        self.end_funcs = new_end_funcs

    def shift_exports(self):
        def get_func_num(f):
            return re.search(EXPORT_REGEX, f).group(4)

        def inject_new_func_num(f, new_func_num):
            return re.sub(EXPORT_REGEX, lambda x: f'{x.group(1)}{x.group(2)}{x.group(3)}{new_func_num}{x.group(5)}', f)

        new_exports = []
        for e in self.exports:
            if e.find('(func ') > -1:
                func_num = get_func_num(e)
                new_func_num = self.function_symbol_map[func_num]

                new_export = inject_new_func_num(e, new_func_num)

                new_exports.append(new_export)
            else:
                new_exports.append(e)

        self.exports = new_exports

    def shift_globals(self, max_global_var):
        new_globals = []
        global_map = {}
        for g in self.global_vars:
            match = re.search(GLOBAL_REGEX, g)

            new_num = int(max_global_var) + 1
            new_glob = re.sub(GLOBAL_REGEX, lambda x: f'{x.group(1)}{new_num}{x.group(3)}', g)

            global_map[match.group(2)] = new_num

            max_global_var = int(max_global_var) + 1
            new_globals.append(new_glob)

        self.global_vars = new_globals

        new_base_funcs = []
        for f in self.base_funcs:
            func = ''
            for l in f.split('\n'):
                match = re.search(GET_SET_GLOBAL_REGEX, l)
                if match:
                    num = match.group(2)
                    new_num = global_map[num]
                    new_get_set = re.sub(GET_SET_GLOBAL_REGEX, lambda x: f'{x.group(1)}{new_num}{x.group(3)}', l)

                    func += new_get_set + '\n'
                else:
                    func += l + '\n'
            new_base_funcs.append(func)

        self.base_funcs = new_base_funcs

        new_end_funcs = []
        for f in self.end_funcs:
            func = ''
            for l in f.split('\n'):
                match = re.search(GET_SET_GLOBAL_REGEX, l)
                if match:
                    num = match.group(2)
                    new_num = global_map[num]
                    new_get_set = re.sub(GET_SET_GLOBAL_REGEX, lambda x: f'{x.group(1)}{new_num}{x.group(3)}', l)

                    func += new_get_set + '\n'
                else:
                    func += l + '\n'
            new_end_funcs.append(func)

        self.end_funcs = new_end_funcs

    def create_imports_map(self):
        imports_map = {}
        for i in self.imports:
            match = re.search(IMPORT_REGEX, i)
            func_num = match.group(6)
            imports_map[func_num] = True

        self.imports_map = imports_map

    def get_max_import(self):
        max_import = -1
        for f in self.imports:
            match = re.search(IMPORT_REGEX, f)
            if match:
                max_import = match.group(6)

        return max_import
