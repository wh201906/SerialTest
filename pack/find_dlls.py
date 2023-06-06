import subprocess
import re
import sys
import ctypes.util


def get_dependencies(file_path, checked_dlls=None):
    if checked_dlls is None:
        checked_dlls = set()

    # ldd cannot find dlls for 32-bit platform
    result = subprocess.run(
        ["objdump", "-p", file_path], capture_output=True, text=True
    )
    output = result.stdout

    pattern = r"DLL Name:\s*(.+)"
    for line in output.splitlines():
        matches = re.findall(pattern, line, re.IGNORECASE)
        if matches:
            dll_name = matches[0].strip()
            dll_path = ctypes.util.find_library(dll_name)
            if dll_path and dll_path not in checked_dlls:
                checked_dlls.add(dll_path)
                checked_dlls.update(get_dependencies(dll_path, checked_dlls))

    return checked_dlls


def copy_dependencies(file_path):
    dependencies = get_dependencies(file_path)
    print("dependencies:", flush=True)
    print(dependencies, flush=True)

    for dependency in dependencies:
        # ignore dlls in system32
        if "system32" in dependency.lower():
            continue
        subprocess.run(["cp", dependency, "./"])


copy_dependencies(sys.argv[1])
