import subprocess
import re
import sys
import ctypes.util
import concurrent.futures
from datetime import datetime

def get_datetime():
    now = datetime.now()
    return now.strftime("%Y-%m-%d %H:%M:%S.%f")

def get_dependencies(file_path):
    new_dlls = []
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
            if dll_path:
                new_dlls.append(dll_path)

    return new_dlls

def get_all_dependencies(file_path):
    checked_dlls=set()
    with concurrent.futures.ThreadPoolExecutor() as executor:
        try:
            workers_num = executor._max_workers
            print(f"Max {workers_num} workers", flush=True)
        except Exception:
            pass
        futures = [executor.submit(get_dependencies, file_path)]
        while futures:
            for completed in concurrent.futures.as_completed(futures):
                new_dlls = completed.result()
                futures.remove(completed)

                for dll in new_dlls:
                    if dll in checked_dlls:
                        continue
                    futures.append(executor.submit(get_dependencies, dll))
                    checked_dlls.add(dll)

    return checked_dlls

def copy_dependencies(file_path):

    print(f"{get_datetime()} Getting dependencies", flush=True)
    dependencies = get_all_dependencies(file_path)
    print(f"dependencies: {len(dependencies)}", flush=True)
    print(dependencies, flush=True)

    print(f"{get_datetime()} Copying dependencies", flush=True)
    for dependency in dependencies:
        # ignore dlls in system32
        if "system32" in dependency.lower():
            continue
        subprocess.run(["cp", dependency, "./"])
    print(f"{get_datetime()} Done", flush=True)


copy_dependencies(sys.argv[1])
