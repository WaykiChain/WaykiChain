#!/usr/bin/env python3
import json
import re
import subprocess
import sys

from generated_wasm import GeneratedWASM
from regexes import DATA_REGEX, EXPORT_REGEX, FUNC_REGEX
from test_wasm import TestWASM


def main(generated_wasm_file, test_wasm_file, out_wasm_file, map_file):
    def read_wasm_file(wasm_file):
        out = subprocess.run(['eosio-wasm2wast', wasm_file], capture_output=True)
        generated_wast_string = out.stdout.decode('utf-8')
        return generated_wast_string

    def get_map(m_file):
        with open(m_file, 'r') as f:
            name_to_num_map = json.load(f)
            num_to_name_map = {v:k for k, v in name_to_num_map.items()}
            return num_to_name_map

    num_to_name_map = get_map(map_file)

    generated_wasm = GeneratedWASM()
    test_wasm = TestWASM()

    generated_wasm.read_wasm(read_wasm_file(generated_wasm_file))
    test_wasm.read_wasm(read_wasm_file(test_wasm_file))

    type_map = test_wasm.shift_types(generated_wasm.max_type)
    max_func_num = test_wasm.shift_imports(type_map, generated_wasm.max_import)

    max_func_num = generated_wasm.shift_base_funcs(max_func_num)

    num_to_name_map = {
        int(k) + generated_wasm.num_imports_base_functions + 1:
        v for k, v in num_to_name_map.items()
    }

    max_func_num = test_wasm.shift_funcs(type_map, max_func_num)
    test_wasm.shift_calls(type_map)
    exports_map = test_wasm.shift_exports()

    generated_wasm.create_imports_map()
    generated_wasm.shift_funcs(num_to_name_map, max_func_num)

    generated_wasm.shift_calls(num_to_name_map, exports_map)

    generated_wasm.shift_exports()

    test_wasm.shift_elems()
    test_wasm.shift_start()
    max_global = test_wasm.get_max_global()

    generated_wasm.shift_globals(max_global)

    with open(out_wasm_file, 'w') as f:
        f.write(write_merged_wasm(generated_wasm, test_wasm))


def merge_data_section(generated_wasm, test_wasm):
    data = []

    test_wasm_zero = ''
    for d in test_wasm.data:
        match = re.search(DATA_REGEX, d)
        if int(match.group(2)) != 0:
            data.append(d)
        else:
            test_wasm_zero = d
            data.append(d)

    for d in generated_wasm.data:
        match = re.search(DATA_REGEX, d)
        if int(match.group(2)) != 0:
            data.append(d)
        else:
            if not test_wasm_zero:
                data.append(d)

    return data


def write_merged_wasm(generated_wasm, test_wasm):
    out = '(module\n'
    for t in generated_wasm.types:
        out += t + '\n'
    for t in test_wasm.types:
        out += t + '\n'

    for i in generated_wasm.imports:
        out += i + '\n'
    for i in test_wasm.imports:
        out += i + '\n'

    for f in generated_wasm.base_funcs:
        out += f + '\n'

    for f in test_wasm.funcs:
        out += f + '\n'

    for f in generated_wasm.end_funcs:
        out += f + '\n'

    if test_wasm.tables:
        for t in test_wasm.tables:
            out += t + '\n'
    else:
        for t in generated_wasm.tables:
            out += t + '\n'

    if test_wasm.memory:
        for m in test_wasm.memory:
            out += m + '\n'
    else:
        for m in generated_wasm.memory:
            out += m + '\n'

    for g in test_wasm.global_vars:
        out += g + '\n'
    for g in generated_wasm.global_vars:
        out += g + '\n'

    for e in test_wasm.exports:
        out += e + '\n'
    for e in generated_wasm.exports:
        out += e + '\n'

    data = merge_data_section(generated_wasm, test_wasm)

    for d in data:
        out += d + '\n'

    if test_wasm.tables:
        for e in test_wasm.elems:
            out += e + '\n'
    else:
        for e in generated_wasm.elems:
            out += e + '\n'

    if test_wasm.start:
        out += test_wasm.start + '\n'
    elif generated_wasm.start:
        out += generated_wasm.start + '\n'

    out += ')\n'
    return out


if __name__ == "__main__":
    g_wasm_file = sys.argv[1]
    t_wasm_file = sys.argv[2]
    o_wasm_file = sys.argv[3]
    mp_file = sys.argv[4]
    main(g_wasm_file, t_wasm_file, o_wasm_file, mp_file)
