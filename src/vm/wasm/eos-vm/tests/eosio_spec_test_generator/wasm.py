import re

from regexes import FUNC_REGEX, IMPORT_REGEX, TYPE_REGEX

from lookahead import LookAhead

class WASM(object):
    def __init__(self):
        self.types = []
        self.imports = []
        self.funcs = []
        self.tables = []
        self.memory = []
        self.global_vars = []
        self.exports = []
        self.data = []
        self.elems = []
        self.start = ''

        self.max_type = -1
        self.max_import = 0

        self.function_symbol_map = {}

    def read_wasm(self, wast_string):
        lines = wast_string.split('\n')
        last_line = lines[-2][0:-1] # strip the trailing parentheses off.
        new_lines = lines[0:-2]
        new_lines.extend([last_line, ''])
        la_lines = LookAhead(new_lines)

        next(la_lines)  # Skip the "module" line

        while la_lines:
            peek = la_lines.peek

            if re.search(TYPE_REGEX, peek):
                self.get_type(peek)
                next(la_lines)
            elif re.search(IMPORT_REGEX, peek):
                self.get_imports(peek)
                next(la_lines)
            elif re.search(FUNC_REGEX, peek):
                la_lines = self.get_funcs(la_lines)
            elif la_lines.peek.find('(table (;') > -1:
                self.tables.append(peek)
                next(la_lines)
            elif la_lines.peek.find('(memory (;') > -1:
                self.memory.append(peek)
                next(la_lines)
            elif la_lines.peek.find('(global (;') > -1:
                self.global_vars.append(peek)
                next(la_lines)
            elif la_lines.peek.find('(export ') > -1:
                self.exports.append(peek)
                next(la_lines)
            elif la_lines.peek.find('(data ') > -1:
                self.data.append(peek)
                next(la_lines)
            elif la_lines.peek.find('(elem ') > -1:
                self.elems.append(peek)
                next(la_lines)
            elif la_lines.peek.find('(start ') > -1:
                self.start = peek
                next(la_lines)
            else:
                next(la_lines)

    def get_type(self, line):
        type_num = int(re.search(TYPE_REGEX, line).group(2))

        if type_num > self.max_type:
            self.max_type = type_num

        self.types.append(line)

    def get_imports(self, line):
        import_num = int(re.search(IMPORT_REGEX, line).group(6))

        if import_num > self.max_import:
            self.max_import = import_num

        self.imports.append(line)

    def get_funcs(self, lines):
        func = lines.peek
        next(lines)
        while not re.search(FUNC_REGEX, lines.peek) and not non_func(lines.peek):
            func += '\n' + lines.peek
            next(lines)
        self.funcs.append(func)

        return lines


def non_func(line):
    blacklist = ['table', 'memory', 'global']
    blacklist_2 = ['export', 'data', 'elem', 'start']
    for b in blacklist:
        if line.find(f'({b} (;') > -1:
            return True
    for b in blacklist_2:
        if line.find(f'({b} ') > -1:
            return True

    return False
