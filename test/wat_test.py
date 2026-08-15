import platform
import subprocess
import os
import tempfile

sys = platform.system()
BIN_PATH = '../bin/deepvm.exe' if sys == 'Windows' else '../bin/deepvm'

total_failures = 0


def test_wat(name, expected):
    """汇编 name.wat 为临时 .wasm，再运行并断言输出。"""
    global total_failures
    wat_path = os.path.join('wat', name)
    wasm_path = os.path.join(
        tempfile.gettempdir(),
        'deepvm_wat_' + name.replace('.', '_').replace('/', '_') + '.wasm')

    try:
        subprocess.check_call([BIN_PATH, '-o', wasm_path, wat_path],
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError as e:
        total_failures += 1
        print(f"FAIL: {name} assemble failed!")
        return

    try:
        actual = subprocess.check_output(
            [BIN_PATH, wasm_path]).decode('utf-8').strip().replace('\r\n', '\n')
    except subprocess.CalledProcessError as e:
        total_failures += 1
        print(f"FAIL: {name} run failed with exit code {e.returncode}!")
        return

    if actual == str(expected).strip().replace('\r\n', '\n'):
        print("PASS")
    else:
        total_failures += 1
        print(f"FAIL: {name} failed! Expecting {expected} but getting {actual}")


test_wat('add_folded.wat', 3)
test_wat('mul_flat.wat', 35)
test_wat('puts_string.wat', 'hello deeplang\n0')
test_wat('if_else.wat', 100)
test_wat('global_get.wat', 42)
test_wat('memory_store_load.wat', 99)
test_wat('call_indirect.wat', 42)
test_wat('start_section.wat', 5)
test_wat('loop_sum.wat', 55)
test_wat('i64_const.wat', 1)

print(f"\n{total_failures} failure(s)")
exit(1 if total_failures > 0 else 0)
