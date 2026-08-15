import platform
import subprocess
import os
import tempfile

sys = platform.system()
BIN_PATH = '../bin/deepvm.exe' if sys == 'Windows' else '../bin/deepvm'

total_failures = 0


def test_with_path(path, expected=None, returncode=0):
    global total_failures
    # # Check memory leak using "leaks" on Apple Chip
    # if (platform.system() == 'Darwin' and platform.processor() == 'arm'):
    #     try:
    #         subprocess.check_call(
    #             ['leaks', '--atExit', '--', '../bin/deepvm', path], stdout=subprocess.DEVNULL)
    #     except subprocess.CalledProcessError as e:
    #         print(f"FAIL: {path} failed memory leak test!")

    try:
        actual = subprocess.check_output(
            [BIN_PATH, path]).decode('utf-8').strip().replace('\r\n', '\n')
        if (actual == str(expected).strip().replace('\r\n', '\n')):
            # print(f"PASS: {path} passed!")
            print("PASS")
        else:
            total_failures += 1
            print(
                f"FAIL: {path} failed! Expecting {expected} but getting {actual}")
    except subprocess.CalledProcessError as e:
        if (returncode == e.returncode):
            # print(
            #     f"PASS: {path} passed with the expected exit code {returncode}!")
            print("PASS")
        else:
            total_failures += 1
            print(f"FAIL: {path} failed with exit code {e.returncode}!\n{e}")


def test_with_wat(name, expected=None, returncode=0):
    """把 test/wat/name 汇编为临时 .wasm 再运行并断言输出。"""
    global total_failures
    wat_path = os.path.join('wat', name)
    wasm_path = os.path.join(
        tempfile.gettempdir(),
        'deepvm_' + name.replace('.', '_').replace('/', '_') + '.wasm')

    try:
        subprocess.check_call(
            [BIN_PATH, '-o', wasm_path, wat_path],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError:
        total_failures += 1
        print(f"FAIL: {name} assemble failed!")
        return

    try:
        actual = subprocess.check_output(
            [BIN_PATH, wasm_path]).decode('utf-8').strip().replace('\r\n', '\n')
        if (actual == str(expected).strip().replace('\r\n', '\n')):
            print("PASS")
        else:
            total_failures += 1
            print(
                f"FAIL: {name} failed! Expecting {expected} but getting {actual}")
    except subprocess.CalledProcessError as e:
        if (returncode == e.returncode):
            print("PASS")
        else:
            total_failures += 1
            print(f"FAIL: {name} failed with exit code {e.returncode}!\n{e}")


test_with_path('math/add_float_0_-9.1201.wasm', 4294967287)
test_with_path('math/add_float_10.2_-8.1.wasm', 2)
test_with_path('math/add_float_10.2_0.0.wasm', 10)
test_with_path('math/add_int_-2_-10.wasm', 4294967284)
test_with_path('math/add_int_0_-10.wasm', 4294967286)
test_with_path('math/add_int_65535_10.wasm', 65545)
test_with_path('math/div_float_-100.88_-0.7.wasm', 144)
test_with_path('math/div_float_0_-23.1.wasm', 0)
test_with_path('math/div_float_22.7_-0.7.wasm', 4294967264)
test_with_path('math/div_float_22.7_0.wasm', 1)
test_with_path('math/div_float_22.7_11.3.wasm', 2)
test_with_path('math/div_int_-2_-23.wasm', 0)
test_with_path('math/div_int_-46_-23.wasm', 2)
test_with_path('math/div_int_0_-23.wasm', 0)
test_with_path('math/div_int_100_0.wasm', returncode=1)
test_with_path('math/mod_int_11_7.wasm', 4)
test_with_path('math/mod_int_100_0.wasm', returncode=1)
test_with_path('math/mult_float_-100.88_-0.0.wasm', 0)
test_with_path('math/mult_float_-100.88_-1.1.wasm', 110)
test_with_path('math/mult_float_-100.88_0.0.wasm', 0)
test_with_path('math/mult_float_99.01_-0.96.wasm', 99)
test_with_path('math/mult_float_99.01_-1.23456.wasm', 4294967174)
test_with_path('math/mult_int_-1_10.wasm', 4294967286)
test_with_path('math/mult_int_-2_-23.wasm', 46)
test_with_path('math/mult_int_0_10.wasm', 0)
test_with_path('math/sub_float_0.0_8.96.wasm', 4294967288)
test_with_path('math/sub_float_10.2_-8.1.wasm', 18)
test_with_path('math/sub_float_10.2_0.0.wasm', 10)
test_with_path('math/sub_float_99.01_22.3.wasm', 76)
test_with_path('math/sub_int_-10_-8.wasm', 4294967294)
test_with_path('math/sub_int_0_-10.wasm', 4294967286)
test_with_path('math/sub_int_65535_10.wasm', 65525)

test_with_path('math/add_int64_0_-10.wasm', -10)
test_with_path('math/add_int64_res_-1.wasm', -1)
test_with_path('math/add_int64_21474836480000_21474.wasm', 21474836501474)
test_with_path('math/add_float64_0_-9.1201.wasm', -9)
test_with_path('math/add_float64_10.2_-8.1.wasm', 2)
test_with_path('math/add_float64_12.3_3.85.wasm', 16)
test_with_path('math/sub_int64_64_-8.wasm', 72)
test_with_path('math/sub_int64_2147483647_65535.wasm', 2147418112)
test_with_path(
    'math/sub_int64_2147483647214748364_6553565535655356553.wasm', -4406081888440608189)
test_with_path('math/sub_float64_12.3_3.85.wasm', 8)
test_with_path('math/sub_float64_12.34561237_19953.85.wasm', -19941)
test_with_path('math/sub_float64_2023.7_0.0.wasm', 2023)
test_with_path('math/sub_float64_0.0_2023.7.wasm', -2023)
test_with_path('math/mult_int64_1024_-65535.wasm', -67107840)
test_with_path('math/mult_int64_-5_0.wasm', 0)
test_with_path('math/mult_float64_0.0_2023.7.wasm', 0)
test_with_path('math/mult_float64_2147483647.15535_2023202420.951468.wasm',
               4344794113878392320)
test_with_path('math/mult_float64_-19.6667_0.0000003.wasm',
               0)  # -0.00000590001
test_with_path('math/div_int64_-5_0.wasm', returncode=1)
test_with_path('math/div_int64_0_-5.wasm', 0)
test_with_path('math/div_int64_214748364_233.wasm', 921666)
test_with_path('math/div_int64_214748364_-2147483648.wasm', 0)
test_with_path('math/div_int64_2147483648_-100.wasm', -21474836)
test_with_path('math/div_float64_5.36_0.00.wasm', 1)
test_with_path('math/div_float64_5.36_0.000000000002.wasm',
               2680000000000)  # 2,680,000,000,000
test_with_path('math/div_float64_0.000000000002_5.36.wasm', 0)
test_with_path('math/div_float64_0.000_5.36.wasm', 0)
test_with_path('math/div_float64_-17.45_5.36.wasm', -3)
test_with_path('math/div_float64_5.36_-17.45.wasm', 0)
test_with_path('math/div_float64_0.00_0.0.wasm', 1)
test_with_path('math/mod_int64_11_4.wasm', 3)
test_with_path('math/mod_int64_2147483648_-10000000.wasm', 7483648)
test_with_path('math/mod_int64_2147483648_1000.wasm', 648)

test_with_path('builtin/builtin_puts_00001.wasm', 'hello deeplang\n0')
test_with_path('builtin/builtin_puti_00001.wasm', 'add(7,8)=150')
test_with_path('builtin/builtin_putf_00001.wasm', 'add(7.1,8.2)=15.2999990')
test_with_path('builtin/builtin_putl_00001.wasm', 'add(7,8)=150')
test_with_path('builtin/builtin_putd_00001.wasm', 'add(7.1,8.2)=15.3000000')

test_with_path('control/if_001.wasm', '10')
test_with_path('control/if_002.wasm', '20')
test_with_path('control/loop_001.wasm', '55')
test_with_path('control/switch_case_001.wasm', '65')
test_with_path('control/tri_if_001.wasm', '30')
test_with_path('control/tri_if_002.wasm', '60')

# shift/rotate（移位量按位宽取模；i32 按无符号、i64 按有符号打印）
test_with_path('shift/i32_shl_001.wasm', 16)
test_with_path('shift/i32_shl_002.wasm', 1)
test_with_path('shift/i32_shl_003.wasm', 3840)
test_with_path('shift/i32_shrs_001.wasm', 4294967292)
test_with_path('shift/i32_shrs_002.wasm', 4294967295)
test_with_path('shift/i32_shrs_003.wasm', 1)
test_with_path('shift/i32_shru_001.wasm', 2147483644)
test_with_path('shift/i32_shru_002.wasm', 1)
test_with_path('shift/i32_shru_003.wasm', 4)
test_with_path('shift/i64_shl_001.wasm', 1099511627776)
test_with_path('shift/i64_shl_002.wasm', -9223372036854775808)
test_with_path('shift/i64_shl_003.wasm', 3840)
test_with_path('shift/i64_shrs_001.wasm', -4)
test_with_path('shift/i64_shrs_002.wasm', -1)
test_with_path('shift/i64_shrs_003.wasm', 1)
test_with_path('shift/i64_shru_001.wasm', 9223372036854775807)
test_with_path('shift/i64_shru_002.wasm', 1)
test_with_path('shift/i64_shru_003.wasm', 4)
test_with_path('shift/i32_rotl_001.wasm', 3)
test_with_path('shift/i32_rotl_002.wasm', 878082066)
test_with_path('shift/i32_rotl_003.wasm', 2147483648)
test_with_path('shift/i32_rotr_001.wasm', 2147483648)
test_with_path('shift/i32_rotr_002.wasm', 1073741824)
test_with_path('shift/i32_rotr_003.wasm', 2014458966)
test_with_path('shift/i64_rotl_001.wasm', 3)
test_with_path('shift/i64_rotl_002.wasm', -9223372036854775808)
test_with_path('shift/i64_rotl_003.wasm', 6230889152035885620)
test_with_path('shift/i64_rotr_001.wasm', -9223372036854775808)
test_with_path('shift/i64_rotr_002.wasm', 4611686018427387904)
test_with_path('shift/i64_rotr_003.wasm', -3607644760502136661)

# memory/（窄宽度 load/store 及 memory.size/grow）
test_with_path('memory/i32_load8_s_001.wasm', 4294967168)
test_with_path('memory/i32_load8_s_002.wasm', 4294967295)
test_with_path('memory/i32_load8_s_003.wasm', 127)
test_with_path('memory/i32_load8_u_001.wasm', 128)
test_with_path('memory/i32_load8_u_002.wasm', 255)
test_with_path('memory/i32_load8_u_003.wasm', 0)
test_with_path('memory/i32_load16_s_001.wasm', 4294934528)
test_with_path('memory/i32_load16_s_002.wasm', 4294967295)
test_with_path('memory/i32_load16_s_003.wasm', 32767)
test_with_path('memory/i32_load16_u_001.wasm', 32768)
test_with_path('memory/i32_load16_u_002.wasm', 65535)
test_with_path('memory/i32_load16_u_003.wasm', 4660)
test_with_path('memory/i64_load8_s_001.wasm', -128)
test_with_path('memory/i64_load8_s_002.wasm', -1)
test_with_path('memory/i64_load8_s_003.wasm', 127)
test_with_path('memory/i64_load8_u_001.wasm', 128)
test_with_path('memory/i64_load8_u_002.wasm', 255)
test_with_path('memory/i64_load8_u_003.wasm', 0)
test_with_path('memory/i64_load16_s_001.wasm', -32768)
test_with_path('memory/i64_load16_s_002.wasm', -1)
test_with_path('memory/i64_load16_s_003.wasm', 32767)
test_with_path('memory/i64_load16_u_001.wasm', 32768)
test_with_path('memory/i64_load16_u_002.wasm', 65535)
test_with_path('memory/i64_load16_u_003.wasm', 4660)
test_with_path('memory/i64_load32_s_001.wasm', -2147483648)
test_with_path('memory/i64_load32_s_002.wasm', -1)
test_with_path('memory/i64_load32_s_003.wasm', 2147483647)
test_with_path('memory/i64_load32_u_001.wasm', 2147483648)
test_with_path('memory/i64_load32_u_002.wasm', 4294967295)
test_with_path('memory/i64_load32_u_003.wasm', 305419896)
test_with_path('memory/i32_store8_001.wasm', 18)
test_with_path('memory/i32_store8_002.wasm', 255)
test_with_path('memory/i32_store8_003.wasm', 0)
test_with_path('memory/i32_store16_001.wasm', 4660)
test_with_path('memory/i32_store16_002.wasm', 65535)
test_with_path('memory/i32_store16_003.wasm', 0)
test_with_path('memory/i64_store8_001.wasm', 18)
test_with_path('memory/i64_store8_002.wasm', 255)
test_with_path('memory/i64_store8_003.wasm', 0)
test_with_path('memory/i64_store16_001.wasm', 4660)
test_with_path('memory/i64_store16_002.wasm', 65535)
test_with_path('memory/i64_store16_003.wasm', 0)
test_with_path('memory/i64_store32_001.wasm', 305419896)
test_with_path('memory/i64_store32_002.wasm', 4294967295)
test_with_path('memory/i64_store32_003.wasm', 0)
test_with_path('memory/memory_size_001.wasm', 1)
test_with_path('memory/memory_size_002.wasm', 2)
test_with_path('memory/memory_size_003.wasm', 3)
test_with_path('memory/memory_grow_001.wasm', 1)
test_with_path('memory/memory_grow_002.wasm', 2)
test_with_path('memory/memory_grow_003.wasm', 3)
test_with_path('memory/memory_grow_pos_001.wasm', 4294967295)

# global/（Global section 的 global.get/set）
test_with_path('global/global_get_001.wasm', 42)
test_with_path('global/global_get_002.wasm', 4294967295)
test_with_path('global/global_get_003.wasm', 4294967296)
test_with_path('global/global_set_001.wasm', 100)
test_with_path('global/global_set_002.wasm', 4294967295)
test_with_path('global/global_set_003.wasm', 4294967296)

# table/（Table/Elem section 与 call_indirect）
test_with_path('table/call_indirect_001.wasm', 42)
test_with_path('table/call_indirect_002.wasm', 100)
test_with_path('table/call_indirect_003.wasm', 7)
test_with_path('table/call_indirect_oob_001.wasm', returncode=1)

# start/（Start section：main 前先执行起始函数）
test_with_path('start/set_global.wasm', 99)

# wat/（WAT 文本汇编为 wasm 再运行，覆盖折叠/平铺及各类 section）
test_with_wat('add_folded.wat', 3)
test_with_wat('mul_flat.wat', 35)
test_with_wat('puts_string.wat', 'hello deeplang\n0')
test_with_wat('if_else.wat', 100)
test_with_wat('global_get.wat', 42)
test_with_wat('memory_store_load.wat', 99)
test_with_wat('call_indirect.wat', 42)
test_with_wat('start_section.wat', 5)
test_with_wat('loop_sum.wat', 55)
test_with_wat('i64_const.wat', 1)

if total_failures > 0:
    print(f"Total {total_failures} tests failed!")
    exit(1)
