import os, sys, shutil
from win32api import GetFileVersionInfo
from json import load
from re import fullmatch


def getPEVersion(fname):
    try:
        fileInfo = GetFileVersionInfo(fname, '\\')
        version = "V%d.%d.%d" % (fileInfo['FileVersionMS'] / 65536,
                                 fileInfo['FileVersionMS'] % 65536,
                                 fileInfo['FileVersionLS'] / 65536)
    except Exception:
        print("Cannot get version number of", fname)
    return version


os.chdir(sys.path[0])
print("Current Directory:", os.getcwd())
targetName = os.path.abspath(os.getcwd()).split('\\')[-2]
print("Target Name", targetName)

src32Dir = ""
src64Dir = ""
srcAndroidDir = ""
dirList = os.listdir("../")

for i in dirList:
    if not os.path.isdir("../" + i):
        continue
    if not i.startswith("build"):
        continue

    if i.endswith("32_bit-Release"):
        src32Dir = "../" + i
    elif i.endswith("64_bit-Release"):
        src64Dir = "../" + i
    elif "Android" in i and i.endswith("Release"):
        srcAndroidDir = "../" + i

src32Path = src32Dir + "/release/" + targetName + ".exe"
src64Path = src64Dir + "/release/" + targetName + ".exe"
AndroidConf = srcAndroidDir + "/android-build/build/outputs/apk/debug/output.json"
srcAndroidPath = srcAndroidDir + "/android-build/build/outputs/apk/debug/android-build-debug.apk"
print("Target Files:")
print(src32Path)
print(src64Path)
print(srcAndroidPath)

ver32 = getPEVersion(src32Path)
ver64 = getPEVersion(src64Path)
verAndroid = load(open(AndroidConf))[0]["apkData"]["versionName"]
print("Versions:")
print("win32:", ver32)
print("win64:", ver64)
print("Android:", verAndroid)
if ver32 != ver64 or ver64 != verAndroid:
    print("WARNING!")
    print("Version names are not the same!")
dst32Dir = "./" + ver32 + "-win32"
dst64Dir = "./" + ver64 + "-win64"
dst32Path = dst32Dir + "/" + targetName + ".exe"
dst64Path = dst64Dir + "/" + targetName + ".exe"
dstAndroidPath = "./" + verAndroid + "-android.apk"

if os.path.exists(dst32Dir) and os.path.exists(dst32Path):
    print(dst32Path, "exists, replacing...")
    os.remove(dst32Path)
elif not os.path.exists(dst32Dir):
    print(dst32Dir, "doesn't exist, creating...")
    shutil.copytree("./32", dst32Dir)
shutil.copyfile(src32Path, dst32Path)

if os.path.exists(dst64Dir) and os.path.exists(dst64Path):
    print(dst64Path, "exists, replacing...")
    os.remove(dst64Path)
elif not os.path.exists(dst64Dir):
    print(dst64Dir, "doesn't exist, creating...")
    shutil.copytree("./64", dst64Dir)
shutil.copyfile(src64Path, dst64Path)

if os.path.exists(dstAndroidPath) :
    print(dstAndroidPath, "exists, replacing...")
    os.remove(dstAndroidPath)
shutil.copyfile(srcAndroidPath, dstAndroidPath)

use7z = input("Compress?(y/N)")
if fullmatch("[Yy][Ee][Ss]|[Yy]", use7z):
    archive32Path = dst32Dir + ".7z"
    archive64Path = dst64Dir + ".7z"

    if os.path.exists(archive32Path):
        print(archive32Path, "exists, replacing...")
        os.remove(archive32Path)
    os.system("7z a -t7z -mmt8 -mx9 " + archive32Path + " " + dst32Dir)

    if os.path.exists(archive64Path):
        print(archive64Path, "exists, replacing...")
        os.remove(archive64Path)
    os.system("7z a -t7z -mmt8 -mx9 " + archive64Path + " " + dst64Dir)
