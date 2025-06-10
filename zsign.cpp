#include "HeaderFiles/common.h"
#include "HeaderFiles/json.h"
#include "HeaderFiles/openssl.h"
#include "HeaderFiles/macho.h"
#include "HeaderFiles/bundle.h"

#include <libgen.h>
#include <dirent.h>
#include <getopt.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

#include <minizip/unzip.h>
#include <minizip/zip.h>
#include <zlib.h>

using namespace std;

const struct option options[] = {
    {"debug", no_argument, NULL, 'd'},
    {"force", no_argument, NULL, 'f'},
    {"verbose", no_argument, NULL, 'v'},
    {"cert", required_argument, NULL, 'c'},
    {"pkey", required_argument, NULL, 'k'},
    {"prov", required_argument, NULL, 'm'},
    {"password", required_argument, NULL, 'p'},
    {"bundle_id", required_argument, NULL, 'b'},
    {"bundle_name", required_argument, NULL, 'n'},
    {"bundle_version", required_argument, NULL, 'r'},
    {"entitlements", required_argument, NULL, 'e'},
    {"output", required_argument, NULL, 'o'},
    {"zip_level", required_argument, NULL, 'z'},
    {"dylib", required_argument, NULL, 'l'},
    {"weak", no_argument, NULL, 'w'},
    {"install", no_argument, NULL, 'i'},
    {"quiet", no_argument, NULL, 'q'},
    {"help", no_argument, NULL, 'h'},
    {"deletemp", no_argument, NULL, 'x'},
    {}
};

bool unzipFile(const std::string& zipPath, const std::string& destPath) {
    unzFile zipfile = unzOpen(zipPath.c_str());
    if (!zipfile) return false;
    if (unzGoToFirstFile(zipfile) != UNZ_OK) return false;

    do {
        char filename[512];
        unz_file_info fileInfo;
        if (unzGetCurrentFileInfo(zipfile, &fileInfo, filename, sizeof(filename), NULL, 0, NULL, 0) != UNZ_OK) break;

        std::string fullPath = destPath + "/" + filename;

        if (filename[strlen(filename) - 1] == '/') {
            std::filesystem::create_directories(fullPath);
            continue;
        }

        std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());
        if (unzOpenCurrentFile(zipfile) != UNZ_OK) break;

        std::ofstream out(fullPath, std::ios::binary);
        char buf[8192];
        int readBytes;
        while ((readBytes = unzReadCurrentFile(zipfile, buf, sizeof(buf))) > 0) {
            out.write(buf, readBytes);
        }

        out.close();
        unzCloseCurrentFile(zipfile);

    } while (unzGoToNextFile(zipfile) == UNZ_OK);

    unzClose(zipfile);
    return true;
}

bool zipFolder(const std::string& folderPath, const std::string& zipPath, int compressionLevel = Z_BEST_SPEED) {
    zipFile zf = zipOpen(zipPath.c_str(), APPEND_STATUS_CREATE);
    if (!zf) return false;

    for (auto& entry : std::filesystem::recursive_directory_iterator(folderPath)) {
        if (entry.is_directory()) continue;
        std::string relPath = std::filesystem::relative(entry.path(), folderPath).string();
        std::ifstream in(entry.path(), std::ios::binary);
        std::vector<char> buffer((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        zip_fileinfo zi = {};
        zipOpenNewFileInZip(zf, relPath.c_str(), &zi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, compressionLevel);
        zipWriteInFileInZip(zf, buffer.data(), buffer.size());
        zipCloseFileInZip(zf);
    }

    zipClose(zf, NULL);
    return true;
}

int main(int argc, char *argv[]) {
    ZTimer gtimer;
    bool excludeProvisioning = false, bForce = false, bInstall = false, bWeakInject = false;
    uint32_t uZipLevel = 0;
    string strCertFile, strPKeyFile, strProvFile, strPassword, strBundleId, strBundleVersion, strDyLibFile, strOutputFile, strDisplayName, strEntitlementsFile;
    string zsignTmpPath = "/tmp/";
    int opt = 0, argslot = -1;

    while (-1 != (opt = getopt_long(argc, argv, "dfvhc:k:m:o:ip:e:b:n:z:ql:wx", options, &argslot))) {
        switch (opt) {
        case 'd': ZLog::SetLogLever(ZLog::E_DEBUG); break;
        case 'f': bForce = true; break;
        case 'c': strCertFile = optarg; break;
        case 'k': strPKeyFile = optarg; break;
        case 'm': strProvFile = optarg; break;
        case 'p': strPassword = optarg; break;
        case 'b': strBundleId = optarg; break;
        case 'r': strBundleVersion = optarg; break;
        case 'n': strDisplayName = optarg; break;
        case 'e': strEntitlementsFile = optarg; break;
        case 'l': strDyLibFile = optarg; break;
        case 'i': bInstall = true; break;
        case 'o': strOutputFile = GetCanonicalizePath(optarg); break;
        case 'z': uZipLevel = atoi(optarg); break;
        case 'w': bWeakInject = true; break;
        case 'q': ZLog::SetLogLever(ZLog::E_NONE); break;
        case 'x': excludeProvisioning = true; break;
        case 'v': printf("version: 0.5\n"); return 0;
        case 'h': case '?': return usage();
        }
    }

    if (optind >= argc) return usage();

    string strPath = GetCanonicalizePath(argv[optind]);
    if (!IsFileExists(strPath.c_str())) {
        ZLog::ErrorV(">>> Invalid Path! %s\n", strPath.c_str());
        return -1;
    }

    bool bZipFile = false;
    if (!IsFolder(strPath.c_str())) {
        bZipFile = IsZipFile(strPath.c_str());
        if (!bZipFile) {
            ZMachO macho;
            if (macho.Init(strPath.c_str())) {
                if (!strDyLibFile.empty()) {
                    bool bCreate = false;
                    macho.InjectDyLib(bWeakInject, strDyLibFile.c_str(), bCreate);
                } else {
                    macho.PrintInfo();
                }
                macho.Free();
            }
            return 0;
        }
    }

    ZTimer timer;
    ZSignAsset zSignAsset;
    if (!zSignAsset.Init(strCertFile, strPKeyFile, strProvFile, strEntitlementsFile, strPassword)) return -1;

    bool bEnableCache = true;
    string strFolder = strPath;
    if (bZipFile) {
        bForce = true; bEnableCache = false;
        string tmpFolder = zsignTmpPath + "zsign_folder_" + to_string(timer.Reset());
        ZLog::PrintV(">>> Unzip: %s -> %s ... \n", strPath.c_str(), tmpFolder.c_str());
        RemoveFolder(tmpFolder.c_str());
        if (!unzipFile(strPath, tmpFolder)) {
            ZLog::Error(">>> Unzip Failed!\n");
            return -1;
        }
        strFolder = tmpFolder;
        timer.PrintResult(true, ">>> Unzip OK!");
    }

    timer.Reset();
    ZAppBundle bundle;
    bool bRet = bundle.SignFolder(&zSignAsset, strFolder, strBundleId, strBundleVersion, strDisplayName, strDyLibFile, bForce, bWeakInject, bEnableCache, excludeProvisioning);
    timer.PrintResult(bRet, ">>> Signed %s!", bRet ? "OK" : "Failed");

    if (!strOutputFile.empty()) {
        timer.Reset();
        size_t pos = bundle.m_strAppFolder.rfind("/Payload");
        if (pos == string::npos) {
            ZLog::Error(">>> Can't Find Payload Directory!\n");
            return -1;
        }
        string baseFolder = bundle.m_strAppFolder.substr(0, pos);
        ZLog::PrintV(">>> Archiving: %s ... \n", strOutputFile.c_str());
        if (!zipFolder(baseFolder + "/Payload", strOutputFile, uZipLevel > 9 ? 9 : uZipLevel)) {
            ZLog::Error(">>> Archive Failed!\n");
            return -1;
        }
        timer.PrintResult(true, ">>> Archive OK! (%s)", GetFileSizeString(strOutputFile.c_str()).c_str());
    }

    if (bRet && bInstall) {
        SystemExec("ideviceinstaller -i '%s'", strOutputFile.c_str());
    }

    string findPath = zsignTmpPath + "zsign_tmp_";
    if (strOutputFile.find(findPath) == 0) RemoveFile(strOutputFile.c_str());
    findPath = zsignTmpPath + "zsign_folder_";
    if (strFolder.find(findPath) == 0) RemoveFolder(strFolder.c_str());

    gtimer.Print(">>> Done.");
    return bRet ? 0 : -1;
}
