#!/usr/bin/env python3
from multiprocessing import Pool
from pathlib import Path

import os
import shutil
import subprocess
import sys
import tempfile

import compile_eosio_tests
import generate_eosio_tests

WASM_DIR = ''
EOSIO_DIR = ''
OUT_DIR = ''
GENERATOR_DIR = ''
REPO_ROOT = ''
ALTERED_WASMS_DIR = ''

generator = ''

test_failures = []

def main():
    json_files = sorted(list(filter(lambda x: x.find('json') > -1, os.listdir(WASM_DIR))))
    try:
        os.mkdir(OUT_DIR)
    except Exception:
        pass

    for j in json_files:
        setup_tests(j)

def get_altered_wasms():
    aws = {}
    for d in os.listdir(ALTERED_WASMS_DIR):
        for f in os.listdir(os.path.join(ALTERED_WASMS_DIR, d)):
            if f.find('.wasm') > -1:
                aws[f] = True
    return aws



def setup_tests(j):
    print(j)
    dir_name = j.split('.')[0]
    new_dir = os.path.join(OUT_DIR, dir_name)
    json_file = os.path.join(WASM_DIR, j)

    _cwd = os.getcwd()

    os.mkdir(new_dir)
    os.chdir(new_dir)

    out = subprocess.run([generator, json_file], capture_output=True)
    out.check_returncode()

    mkdirs()
    copy(dir_name)
    compile_wasm()
    generate_wasm_and_copy()
    copy_cpp()

    os.chdir(_cwd)


def mkdirs():
    new_dirs = []
    for f in os.listdir():
        num = f.split('.')[1]
        if num.isdigit():
            new_dirs.append(num)

    for d in set(new_dirs):
        os.mkdir(str(d))

    for f in os.listdir():
        if not os.path.isdir(f):
            num = f.split('.')[1]
            if num.isdigit():
                os.rename(f, os.path.join(num, f))


def copy(dir_name):
    for d in os.listdir():
        if d.isdigit():
            shutil.copy(
                os.path.join(WASM_DIR, f'{dir_name}.{d}.wasm'),
                os.path.join(os.getcwd(), d, 'test.wasm')
            )


def compile_wasm():
    cwd = os.getcwd()
    name = cwd.split('/')[-1]
    fs = os.listdir()

    fs = list(filter(lambda x: os.path.isdir(x), fs))
    fs_m = map(lambda x: (x, name), fs)

    # If there's a lot of files, break out and process in parallel.
    # Otherwise, we can just do serially.
    if len(fs) > 5:
        with Pool(os.cpu_count() - 2) as p:
            p.map(compile_eosio, fs_m)
    else:
        for d in fs_m:
            compile_eosio(d)

def compile_eosio(f):
    d, name = f
    compile_eosio_tests.main(
        d,
        f'{name}.{d}.wasm.cpp',
        f'{name}.{d}-int.wasm',
    )


def generate_wasm_and_copy():
    cwd = os.getcwd()
    name = cwd.split('/')[-1]
    altered_wasms = get_altered_wasms()
    for d in os.listdir():
        if not d.isdigit():
            continue

        wasm_file = f'{name}.{d}.wasm'
        if wasm_file in altered_wasms:
            wasm_file_path = os.path.join(ALTERED_WASMS_DIR, name, wasm_file)
        else:
            try:
                g_wasm_file = os.path.join(d, f'{name}.{d}-int.wasm')
                t_wasm_file = os.path.join(d, 'test.wasm')
                o_wast_file = os.path.join(d, f'{name}.{d}.wast')
                map_file = os.path.join(d, f'{name}.{d}.wasm.map')
                generate_eosio_tests.main(g_wasm_file, t_wasm_file, o_wast_file, map_file)
                wasm_file_path = os.path.join(d, wasm_file)
                out = subprocess.run(
                    ['eosio-wast2wasm', o_wast_file, '-o', os.path.join(d, wasm_file)],
                    capture_output=True
                )
                if out.returncode > 0:
                    print('    ', o_wast_file, 'failed to compile')
            except Exception:
                continue

        test_dir = os.path.join(EOSIO_DIR, 'wasm_spec_tests')

        shutil.copy(wasm_file_path, os.path.join(test_dir, 'wasms', wasm_file))


def copy_cpp():
    cwd = os.getcwd()
    name = cwd.split('/')[-1]
    cpp_file = f'{name}.cpp'
    test_dir = os.path.join(EOSIO_DIR, 'wasm_spec_tests')
    try:
        shutil.copy(cpp_file, os.path.join(test_dir, cpp_file))
    except FileNotFoundError:
        # This occurs when a test suite is all `assert_malformed` or other tests we don't test.
        pass

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("""Please provide:
                Arg 1: Directory containing test wasms
                Arg 2: Directory containing eosio repo
                (optional) Arg 3: Directory for intermediate test files <defaults to a temp directory>

                ex:
                python setup_eosio_tests.py ~/code/eos-vm-test-wasms ~/code/eos ~/wasm_spec_tests
              """)
        sys.exit(1)

    WASM_DIR = sys.argv[1]
    EOSIO_DIR = sys.argv[2]
    if len(sys.argv) > 3:
        OUT_DIR = sys.argv[3]
    else:
        OUT_DIR = tempfile.mkdtemp()

    print(f'Temporary files will be saved in {OUT_DIR}')

    REPO_ROOT = Path(os.path.realpath(__file__)).parent.parent.parent
    GENERATOR_DIR = os.path.join(REPO_ROOT, 'build', 'tests')
    ALTERED_WASMS_DIR = os.path.join(REPO_ROOT, 'tests', 'eosio_spec_test_generator', 'altered-wasms')

    generator = os.path.join(GENERATOR_DIR, 'eosio_test_generator')

    test_failures = []
    main()
