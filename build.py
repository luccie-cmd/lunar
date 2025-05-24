#!/bin/python3
import os
import glob
import sys
import subprocess
import threading
from script.util import parseSize, compareFiles

def readConfig(path: str) -> dict[str, list[str]]:
    """Reads a configuration file and returns a dictionary of lists of strings."""
    config = {}
    try:
        with open(path, "r") as f:
            lines = f.readlines()
            for line in lines:
                line = line.strip()
                if ':' in line:
                    key, value = line.split(":", 1)
                    key = key.strip()
                    value = value.strip().strip("'")
                    config[key] = [v.strip().strip("'") for v in value.split(",")]
    except FileNotFoundError:
        print(f"Error: Configuration file '{path}' not found.")
        exit(1)
    return config


def writeConfig(config: dict[str, list[str]], path: str) -> None:
    """Writes a configuration dictionary to a file."""
    with open(path, "w") as f:
        for key, values in config.items():
            f.write(f"{key}:")
            for val in enumerate(values):
                f.write(f"'{val[1]}'")
                if val[0]+1 < len(values):
                    f.write(", ")
            f.write('\n')

def checkConfig(config: dict[str, list[str]], allowed_config: list[list[str, list[str], bool]]) -> bool:
    for (key, valid_values, is_mandatory) in allowed_config:
        if key not in config:
            if is_mandatory:
                print(f"Mandatory config {key} is missing")
                return False
            continue
        if valid_values:
            for value in config[key]:
                if value not in valid_values:
                    print(f"Invalid value '{value}' for key '{key}'")
                    return False
    return True

CONFIG = readConfig("./script/config.py")
OLD_CONFIG = {}
if os.path.exists("./script/config.py.old"):
    OLD_CONFIG = readConfig("./script/config.py.old")
ALLOWED_CONFIG = [
    ["config", ["release", "debug"], True],
    ["compiler", ["clang", "gcc"], True],
    ["outDir", [], True],
    ["analyzer", ["yes", "no"], True],
    ["lto", ["yes", "no"], True],
    ["asan", ["yes", "no"], True],
    ["usan", ["yes", "no"], True]
]
if not checkConfig(CONFIG, ALLOWED_CONFIG):
    print("Invalid config file.")
    print("Allowed config items")
    for option in ALLOWED_CONFIG:
        name = option[0]
        values = option[1]
        required = option[2]
        print(f"{name} (required = {required})")
        if len(values) == 0:
            print("    This can be anything as long as it's provided")
        else:
            for val in values:
                print(f"  - {val}")
    exit(1)
writeConfig(CONFIG, "./script/config.py.old")
force_rebuild = False
if OLD_CONFIG != CONFIG:
    force_rebuild = True
    print("Configuration changed, rebuilding...")
CONFIG["CFLAGS"] = ['-c', '-DCOMPILE', '-fno-omit-frame-pointer', '-g0', '-funsafe-math-optimizations -ffast-math']
CONFIG["CFLAGS"] += ["-O0", '-DNDEBUG']
CONFIG["CFLAGS"] += ['-Werror', '-Wall', '-Wextra', '-Wpointer-arith', '-Wshadow', '-Wuninitialized', '-Wno-unneeded-internal-declaration']
CONFIG["CXXFLAGS"] = ['-fno-exceptions', '-fno-rtti']
CONFIG["ASFLAGS"] = ['-c']
CONFIG["LDFLAGS"] = ['-Wl,--gc-sections', '-Wl,--build-id=none', '-O0', '-march=native', '-mtune=native']
CONFIG["INCPATHS"] = ['-Iinclude']

if "gcc" in CONFIG.get("compiler"):
    CONFIG["CFLAGS"] += ['-fmax-errors=1']

if "yes" in CONFIG.get("analyzer"):
    CONFIG["CFLAGS"].append("-fanalyzer")
    if "yes" in CONFIG.get("lto"):
        CONFIG["LDFLAGS"].append("-fanalyzer")

elif "clang" in CONFIG.get("compiler"):
    CONFIG["LDFLAGS"] += ['-fuse-ld=lld']

if "yes" in CONFIG.get("lto"):
    CONFIG["CFLAGS"] += ['-flto']
    CONFIG["LDFLAGS"] += ['-flto=auto']
else:
    CONFIG["CFLAGS"] += ['-fno-lto']
    CONFIG["LDFLAGS"] += ['-fno-lto']

if "yes" in CONFIG.get("asan"):
    CONFIG["CFLAGS"] += ['-fsanitize=address']
    CONFIG["LDFLAGS"] += ['-fsanitize=address']

if "yes" in CONFIG.get("usan"):
    CONFIG["CFLAGS"] += ['-fsanitize=undefined']
    CONFIG["LDFLAGS"] += ['-fsanitize=undefined']

stopEvent = threading.Event()

def callCmd(command, print_out=False):
    with open("commands.txt", "a") as f:
        f.write(command+'\n')
    result = subprocess.run(command, capture_output=not print_out, text=True, shell=True)
    if result.returncode != 0 and result.stderr != None:
        print(result.stderr)
    return [result.returncode, result.stdout]

callCmd("rm -rf commands.txt")

if not compareFiles('build.py', '.build-cache/build.py'):
    if not os.path.exists(".build-cache"):
        callCmd(f"mkdir -p .build-cache")
    callCmd(f"cp 'build.py' .build-cache/build.py")
    force_rebuild = True

def removeOptnone(file):
    callCmd(f"sed s/optnone//g {file} > {file}.clean")
    callCmd(f"sed s/noinline//g {file}.clean > {file}.clean2")
    callCmd(f"mv {file}.clean2 {file}.clean")


def checkExtension(file: str, valid_extensions: list[str]):
    for ext in valid_extensions:
        if file.endswith(ext):
            return True
    return False

def getExtension(file):
    return file.split(".")[-1]

optPasses = [
    'always-inline',
    'attributor',
    'called-value-propagation',
    'canonicalize-aliases',
    'constmerge',
    'coro-cleanup',
    'coro-early',
    'cross-dso-cfi',
    'deadargelim',
    'elim-avail-extern',
    'extract-blocks',
    'expand-variadics',
    'forceattrs',
    'globalopt',
    'globalsplit',
    'hotcoldsplit',
    'inferattrs',
    'inliner-wrapper',
    'inliner-wrapper-no-mandatory-first',
    'instrprof',
    'iroutliner',
    'mergefunc',
    'module-inline',
    'partial-inliner',
    'pgo-force-function-attrs',
    'pgo-icall-prom',
    'pre-isel-intrinsic-lowering',
    'recompute-globalsaa',
    'rel-lookup-table-converter',
    'scc-oz-module-inliner',
    'synthetic-counts-propagation',
    'wholeprogramdevirt',
    'loop-extract',
    'ipsccp',
    'globaldce',
    'argpromotion',
    'attributor-cgscc',
    'coro-split',
    'function-attrs',
    'inline',
    'adce',
    'aggressive-instcombine',
    'alignment-from-assumptions',
    'assume-builder',
    'assume-simplify',
    'bdce',
    'callsite-splitting',
    'codegenprepare',
    'consthoist',
    'constraint-elimination',
    'coro-elide',
    'correlated-propagation',
    'dce',
    'declare-to-assign',
    'dfa-jump-threading',
    'div-rem-pairs',
    'dse',
    'expand-memcmp',
    'fix-irreducible',
    'flatten-cfg',
    'guard-widening',
    'gvn-hoist',
    'gvn-sink',
    'indirectbr-expand',
    'infer-alignment',
    'instsimplify',
    'interleaved-access',
    'interleaved-load-combine',
    'jump-threading',
    'jump-table-to-switch',
    'lcssa',
    'libcalls-shrinkwrap',
    'load-store-vectorizer',
    'loop-data-prefetch',
    'loop-distribute',
    'loop-fusion',
    'loop-load-elim',
    'loop-simplify',
    'loop-sink',
    'loop-versioning',
    'lower-constant-intrinsics',
    'lower-expect',
    'lower-invoke',
    'lower-switch',
    'lower-widenable-condition',
    'mem2reg',
    'memcpyopt',
    'mergeicmps',
    'mergereturn',
    'nary-reassociate',
    'newgvn',
    'partially-inline-libcalls',
    'pgo-memop-opt',
    'reassociate',
    'scalarize-masked-mem-intrin',
    'sccp',
    'select-optimize',
    'separate-const-offset-from-gep',
    'sink',
    'slp-vectorizer',
    'slsr',
    'tailcallelim',
    'tlshoist',
    'typepromotion',
    'vector-combine',
    'early-cse',
    'loop-flatten',
    'loop-interchange',
    'loop-unroll-and-jam',
    'canon-freeze',
    'indvars',
    'loop-bound-split',
    'loop-deletion',
    'loop-idiom',
    'loop-idiom-vectorize',
    'loop-instsimplify',
    'loop-predication',
    'loop-reduce',
    'loop-simplifycfg',
    'loop-unroll-full',
    'loop-versioning-licm',
]

def buildC(file):
    compiler = CONFIG.get("compiler")[0]
    options = CONFIG["CFLAGS"].copy()
    if "main.c" in file and "yes" in CONFIG.get("analyzer"):
        options.remove("-fanalyzer")
    options.append("-std=c11")
    command = compiler + " " + file
    for option in options:
        command += " " + option
    if compiler == "gcc":
        print(f"CXX   {file}")
        command += f" -o {CONFIG['outDir'][0]}/{file}.o"
        return callCmd(command, True)[0]
    else:
        command += ' -S -emit-llvm'
        command += f" -o {CONFIG['outDir'][0]}/{file}.unopt.ll"
        print(f"CLANG {file}")
        if callCmd(command, True)[0] != 0:
            return 1
        removeOptnone(f"{CONFIG['outDir'][0]}/{file}.unopt.ll")
        command = f"mv {CONFIG['outDir'][0]}/{file}.unopt.ll.clean {CONFIG['outDir'][0]}/{file}.unopt.ll"
        if callCmd(command, True)[0] != 0:
            return 1
        command = f"rm -f {CONFIG['outDir'][0]}/{file}.unopt.ll.clean"
        if callCmd(command, True)[0] != 0:
            return 1
        command = f"opt \"{CONFIG['outDir'][0]}/{file}.unopt.ll\" -o \"{CONFIG['outDir'][0]}/{file}.opt.bc\" --strip-debug --strip-named-metadata"
        command += " -passes="
        for pass_ in optPasses:
            command += f"{pass_}"
            if pass_ != optPasses[-1]:
                command += ','
        print(f"OPT   {file}")
        code = callCmd(command, True)[0]
        if code != 0:
            return code
        command = f"llvm-dis {CONFIG['outDir'][0]}/{file}.opt.bc -o {CONFIG['outDir'][0]}/{file}.opt.ll"
        if callCmd(command, True)[0] != 0:
            return 1
        command = f"llc {CONFIG['outDir'][0]}/{file}.opt.ll -o {CONFIG['outDir'][0]}/{file}.opt.asm --filetype=asm --x86-asm-syntax=intel"
        if callCmd(command, True)[0] != 0:
            return 1
        return code

def buildCXX(file):
    compiler = CONFIG.get("compiler")[0]
    if compiler == "gcc":
        compiler = "g"
    compiler += "++"
    options = CONFIG["CFLAGS"].copy()
    options += CONFIG["CXXFLAGS"].copy()
    if "main.c" in file and "yes" in CONFIG.get("analyzer"):
        options.remove("-fanalyzer")
    options.append("-std=c++23")
    command = compiler + " " + file
    for option in options:
        command += " " + option
    if compiler == "g++":
        print(f"CXX   {file}")
        command += f" -o {CONFIG['outDir'][0]}/{file}.o"
        return callCmd(command, True)[0]
    else:
        command += ' -S -emit-llvm'
        command += f" -o {CONFIG['outDir'][0]}/{file}.unopt.ll"
        print(f"CLANG {file}")
        if callCmd(command, True)[0] != 0:
            return 1
        removeOptnone(f"{CONFIG['outDir'][0]}/{file}.unopt.ll")
        command = f"mv {CONFIG['outDir'][0]}/{file}.unopt.ll.clean {CONFIG['outDir'][0]}/{file}.unopt.ll"
        if callCmd(command, True)[0] != 0:
            return 1
        command = f"rm -f {CONFIG['outDir'][0]}/{file}.unopt.ll.clean"
        if callCmd(command, True)[0] != 0:
            return 1
        command = f"opt \"{CONFIG['outDir'][0]}/{file}.unopt.ll\" -o \"{CONFIG['outDir'][0]}/{file}.opt.bc\" --strip-debug --strip-named-metadata"
        command += " -passes="
        for pass_ in optPasses:
            command += f"{pass_}"
            if pass_ != optPasses[-1]:
                command += ','
        print(f"OPT   {file}")
        code = callCmd(command, True)[0]
        if code != 0:
            return code
        command = f"llvm-dis {CONFIG['outDir'][0]}/{file}.opt.bc -o {CONFIG['outDir'][0]}/{file}.opt.ll"
        if callCmd(command, True)[0] != 0:
            return 1
        command = f"llc {CONFIG['outDir'][0]}/{file}.opt.ll -o {CONFIG['outDir'][0]}/{file}.opt.asm --filetype=asm --x86-asm-syntax=intel"
        if callCmd(command, True)[0] != 0:
            return 1
        return code

def buildASM(file):
    compiler = "gcc"
    options = CONFIG["ASFLAGS"].copy()
    command = compiler + " " + file
    for option in options:
        command += " " + option
    print(f"AS    {file}")
    command += f" -o {CONFIG['outDir'][0]}/{file}.o"
    return callCmd(command, True)[0]

def buildAR(dir: str, out_file: str):
    files = glob.glob(f"{dir}/**", recursive=True)
    obj_files = []
    for file in files:
        if not os.path.isfile(file):
            continue
        if not checkExtension(file, ["o"]):
            continue
        obj_files.append(file)
    obj_files_str = " ".join(obj_files)
    cmd = f"ar rcs {out_file} {obj_files_str}"
    print(f"AR    {out_file}")
    callCmd(cmd)

def buildNormal(kernel_dir: str) -> bool:
    files = glob.glob(kernel_dir+'/**', recursive=True)
    CONFIG["INCPATHS"] += [f"-I{kernel_dir}"]
    changed = False
    for file in files:
        if not os.path.isfile(file):
            continue
        if not checkExtension(file, ["c", "cc", "S"]):
            continue
        if getExtension(file) == "inc" or getExtension(file) == "h":
            continue
        basename = os.path.basename(os.path.dirname(os.path.realpath(__file__)))
        str_paths = ""
        for incPath in CONFIG["INCPATHS"]:
            str_paths += f" {incPath}"
        code, _ = callCmd(f"cpp {str_paths} -DCOMPILE -D_GLIBCXX_HOSTED=1 {file} -o ./tmp.txt", True)
        if code != 0:
            print(f"CPP failed to pre process {file}")
            exit(code)
        if not force_rebuild and compareFiles("./tmp.txt", os.path.abspath(f"./.build-cache/{basename}/cache/{file}")):
            continue
        changed = True
        callCmd(f"mkdir -p {CONFIG['outDir'][0]}/{os.path.dirname(file)}")
        callCmd(f"mkdir -p ./.build-cache/{basename}/cache/{os.path.dirname(file)}")
        callCmd(f"cp ./tmp.txt ./.build-cache/{basename}/cache/{file}")
        code = 0
        CONFIG["CFLAGS"] += CONFIG["INCPATHS"]
        CONFIG["ASFLAGS"] += CONFIG["INCPATHS"]
        if getExtension(file) == "c":
            code = buildC(file)
        elif getExtension(file) == "S":
            code = buildASM(file)
        elif getExtension(file) == "cc":
            code = buildCXX(file)
        else:
            print(f"Invalid or unhandled extension `{getExtension(file)}` on file {file}")
            exit(1)

        for incPath in CONFIG["INCPATHS"]:
            CONFIG["CFLAGS"].remove(incPath)
            CONFIG["ASFLAGS"].remove(incPath)

        if code != 0:
            callCmd(f"rm -f ./.build-cache/{basename}/cache/{file}")
            exit(code)
        
        if getExtension(file) == "cc" or getExtension(file) == "c":
            print(f"FMT   {file}")
            callCmd(f"clang-format -i {file}")

    return changed

def linkDir(kernel_dir, use_linker_file: bool, use_libc: bool, out_name, linker_file="", static_lib_files=[]):
    files = glob.glob(kernel_dir+'/**', recursive=True)
    command = "g++"
    options = CONFIG["LDFLAGS"]
    for option in options:
        command += " " + option
    for file in files:
        if not os.path.isfile(file):
            continue
        if not checkExtension(file, ["o", "bc"]):
            continue
        command += " " + file
    if use_linker_file:
        command += f" -Wl,-T {linker_file}"
    if not use_libc:
        command += ' -nostartfiles'
        command += ' -nostdlib'
    for static_lib in static_lib_files:
        command += f" {static_lib}"
    if "debug" in CONFIG.get("config"):
        command += f" -Wl,-Map={CONFIG['outDir'][0]}/{out_name}.map"
    command += f" -o {CONFIG['outDir'][0]}/{out_name}.elf"
    file = f"{CONFIG['outDir'][0]}/{out_name}.elf"
    print(f"LD   {file}")
    if callCmd(command, True)[0] != 0:
        print(f"LD   {file} Failed")
        exit(1)
    callCmd(f"objdump -C -d -Mintel -g -r -t -L {CONFIG['outDir'][0]}/{out_name}.elf > {CONFIG['outDir'][0]}/{out_name}.asm")

def makeImageFile(out_file):
    size = parseSize(CONFIG["imageSize"][0])
    divSize = parseSize("1M")
    command = f"dd if=/dev/zero of={out_file} bs=1M count={size//divSize}"
    print("> Making image file")
    callCmd(command)

def makePartitionTable(out_file):
    print("> Making GPT partition")
    command = f"parted {out_file} --script mklabel gpt"
    callCmd(command)
    print("> Making EFI partition")
    command = f"parted {out_file} --script mkpart EFI FAT32 2048s 100MB"
    callCmd(command)
    print("> Setting EFI partition to be bootable")
    command = f"parted {out_file} --script set 1 boot on"
    callCmd(command)

def setupLoopDevice(out_file):
    print("> Setting up loop device")
    command = f"sudo losetup --show -f -P {out_file} > ./.build-cache/tmp.txt"
    callCmd(command)
    loop_device = ""
    with open("./.build-cache/tmp.txt") as f:
        loop_device = f.readline()
    loop_device = loop_device.strip()
    print(f"> Loop device: {loop_device}")
    return loop_device

def makeFileSystem(loop_device):
    print("> Formatting file systems")
    callCmd(f"sudo mkfs.fat -F32 {loop_device}p1")

def mountFs(device, boot, kernel):
    callCmd(f"mkdir -p mnt")
    print("> Copying boot files")
    callCmd(f"sudo mount {device}p1 mnt")
    callCmd(f"sudo mkdir -p mnt/EFI/BOOT")
    callCmd(f"sudo cp {boot} mnt/EFI/BOOT")
    callCmd(f"sudo cp {kernel} mnt")
    callCmd(f"sudo echo \"Hello, World\" > mnt/test.txt")
    if "limine-uefi" in CONFIG["bootloader"]:
        callCmd(f"sudo cp {CONFIG['outDir'][0]}/limine.conf mnt")
    callCmd(f"sudo umount -l mnt")
    callCmd(f"sudo losetup -d {device}")
    callCmd(f"rm -rf mnt")


def buildImage(out_file, boot_file, kernel_file):
    callCmd(f"rm -f {out_file}")
    makeImageFile(out_file)
    makePartitionTable(out_file)
    LOOP_DEVICE=setupLoopDevice(out_file)
    makeFileSystem(LOOP_DEVICE)
    mountFs(LOOP_DEVICE, boot_file, kernel_file)

def buildStaticLib(directory, out_file):
    os.makedirs(CONFIG["outDir"][0]+'/'+directory, exist_ok=True)
    CONFIG["INCPATHS"] += [f'-I{directory}']
    files = glob.glob(directory+'/**', recursive=True)
    changed = False
    for file in files:
        if not os.path.isfile(file):
            continue
        if not checkExtension(file, ["c", "cc", "asm"]):
            continue
        basename = os.path.basename(os.path.dirname(os.path.realpath(__file__)))
        str_paths = ""
        for incPath in CONFIG["INCPATHS"]:
            str_paths += f" {incPath}"
        code, _ = callCmd(f"cpp {str_paths} -DCOMPILE -D_GLIBCXX_HOSTED=1 {file} -o ./tmp.txt", True)
        if code != 0:
            print(f"CPP failed to pre process {file}")
            exit(code)
        if not force_rebuild and compareFiles("./tmp.txt", os.path.abspath(f"./.build-cache/{basename}/cache/{file}")):
            continue
        changed = True
        callCmd(f"mkdir -p {CONFIG['outDir'][0]}/{os.path.dirname(file)}")
        callCmd(f"mkdir -p ./.build-cache/{basename}/cache/{os.path.dirname(file)}")
        callCmd(f"cp ./tmp.txt ./.build-cache/{basename}/cache/{file}")
        code = 0
        CONFIG["CFLAGS"] += CONFIG["INCPATHS"]
        CONFIG["ASFLAGS"] += CONFIG["INCPATHS"]
        if getExtension(file) == "c":
            code = buildC(file)
        elif getExtension(file) == "asm":
            code = buildASM(file)
        elif getExtension(file) == "cc":
            code = buildCXX(file)
        else:
            print(f"Invalid or unhandled extension {getExtension(file)}")
            exit(1)

        for incPath in CONFIG["INCPATHS"]:
            CONFIG["CFLAGS"].remove(incPath)
            CONFIG["ASFLAGS"].remove(incPath)

        if code != 0:
            callCmd(f"rm -f ./.build-cache/{basename}/cache/{file}")
            exit(code)
        
        if getExtension(file) == "cc" or getExtension(file) == "c":
            print(f"> clang-format {file}")
            callCmd(f"clang-format -i {file}")

    if changed:
        buildAR(f"{CONFIG["outDir"][0]}/{directory}", out_file)
    return changed

def buildDir(directory, static_lib: bool, out_file="a.out"):
    if static_lib:
        return buildStaticLib(directory, out_file)
    else:
        return buildNormal(directory)

def getInfo():
    callCmd("rm -f info.txt")
    callCmd("touch info.txt")
    callCmd(f"cloc . --exclude-dir=bin,.build-cache >> info.txt")
    callCmd(f"tree -I 'bin' -I 'script' -I '.vscode' -I 'tmp.txt' -I 'commands.txt' -I 'info.txt' >> info.txt")

def main():
    basename = os.path.basename(os.path.dirname(os.path.realpath(__file__)))
    if "clean" in sys.argv:
        callCmd(f"rm -rf ./.build-cache/{basename}")
        callCmd(f"rm -rf {CONFIG['outDir'][0]}/")
    if "clean-all" in sys.argv:
        callCmd(f"rm -rf ./.build-cache/{basename}")
        callCmd(f"rm -rf {CONFIG['outDir'][0]}/")
    if force_rebuild:
        print("Rebuilding...")
        callCmd(f"rm -rf ./.build-cache/{basename}")
        callCmd(f"rm -rf {CONFIG['outDir'][0]}/")
    print("> Creating necesarry dirs")
    callCmd(f"mkdir -p {CONFIG['outDir'][0]}")
    if "compile" in sys.argv:
        changed: bool = False
        changed = buildDir("src", False)
        if changed or not os.path.exists(f"{CONFIG['outDir'][0]}/lng.elf"):
            print("> Linking source")
            linkDir(f"{CONFIG['outDir'][0]}/src", False, True, "lng")
        print("> Getting info")
        getInfo()
    currentUser = os.getlogin()
    callCmd(f"chown -R {currentUser}:{currentUser} ./")

if __name__ == '__main__':
    main()