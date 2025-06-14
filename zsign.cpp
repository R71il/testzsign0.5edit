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
    {"frameworks", no_argument, NULL, 'y'}, // تمت إضافة الخيار y
    {}};

int usage()
{
    ZLog::Print("Usage: zsign [-options] [-k privkey.pem] [-m dev.prov] [-o output.ipa] file|folder\n");
    ZLog::Print("options:\n");
    ZLog::Print("-k, --pkey\t\tPath to private key or p12 file. (PEM or DER format)\n");
    ZLog::Print("-m, --prov\t\tPath to mobile provisioning profile.\n");
    ZLog::Print("-c, --cert\t\tPath to certificate file. (PEM or DER format)\n");
    ZLog::Print("-d, --debug\t\tGenerate debug output files. (.zsign_debug folder)\n");
    ZLog::Print("-f, --force\t\tForce sign without cache when signing folder.\n");
    ZLog::Print("-o, --output\t\tPath to output ipa file.\n");
    ZLog::Print("-p, --password\t\tPassword for private key or p12 file.\n");
    ZLog::Print("-b, --bundle_id\t\tNew bundle id to change.\n");
    ZLog::Print("-n, --bundle_name\tNew bundle name to change.\n");
    ZLog::Print("-r, --bundle_version\tNew bundle version to change.\n");
    ZLog::Print("-e, --entitlements\tNew entitlements to change.\n");
    ZLog::Print("-z, --zip_level\t\tCompressed level when output the ipa file. (0-9)\n");
    ZLog::Print("-l, --dylib\t\tPath to inject dylib file.\n");
    ZLog::Print("-w, --weak\t\tInject dylib as LC_LOAD_WEAK_DYLIB.\n");
    ZLog::Print("-y, --frameworks\tInject dylib into Frameworks folder.\n"); // تمت إضافة هذا السطر
    ZLog::Print("-i, --install\t\tInstall ipa file using ideviceinstaller command for test.\n");
    ZLog::Print("-q, --quiet\t\tQuiet operation.\n");
    ZLog::Print("-v, --version\t\tShows version.\n");
    ZLog::Print("-h, --help\t\tShows help (this message).\n");

    return -1;
}

int main(int argc, char *argv[])
{
    ZTimer gtimer;
    
    bool excludeProvisioning = false;
    bool bForce = false;
    bool bInstall = false;
    bool bWeakInject = false;
    bool bInjectToFrameworks = false; // تمت إضافة هذا المتغير
    uint32_t uZipLevel = 0;

    string strCertFile;
    string strPKeyFile;
    string strProvFile;
    string strPassword;
    string strBundleId;
    string strBundleVersion;
    string strDyLibFile;
    string strOutputFile;
    string strDisplayName;
    string strEntitlementsFile;

    int opt = 0;
    int argslot = -1;

    // Set tmp dir path
    string zsignTmpPath;
    // Check if the OS is Windows
#ifdef _WIN32
    zsignTmpPath = "C:/tmp/";
#else
    zsignTmpPath = "/tmp/";
#endif

    while (-1 != (opt = getopt_long(argc, argv, "dfvhc:k:m:o:ip:e:b:n:z:ql:wyx", options, &argslot))) // تمت إضافة y في السلسلة
    {
        switch (opt)
        {
        case 'd':
            ZLog::SetLogLever(ZLog::E_DEBUG);
            break;
        case 'f':
            bForce = true;
            break;
        case 'c':
            strCertFile = optarg;
            break;
        case 'k':
            strPKeyFile = optarg;
            break;
        case 'm':
            strProvFile = optarg;
            break;
        case 'p':
            strPassword = optarg;
            break;
        case 'b':
            strBundleId = optarg;
            break;
        case 'r':
            strBundleVersion = optarg;
            break;
        case 'n':
            strDisplayName = optarg;
            break;
        case 'e':
            strEntitlementsFile = optarg;
            break;
        case 'l':
            strDyLibFile = optarg;
            break;
        case 'i':
            bInstall = true;
            break;
        case 'o':
            strOutputFile = GetCanonicalizePath(optarg);
            break;
        case 'z':
            uZipLevel = atoi(optarg);
            break;
        case 'w':
            bWeakInject = true;
            break;
        case 'y': // تمت إضافة هذا الخيار
            bInjectToFrameworks = true;
            break;
        case 'q':
            ZLog::SetLogLever(ZLog::E_NONE);
            break;
        case 'x':
            excludeProvisioning = true;
            break;
        case 'v':
        {
            printf("version: 0.5\n");
            return 0;
        }
        break;
        case 'h':
        case '?':
            return usage();
            break;
        }

        ZLog::DebugV(">>> Option:\t-%c, %s\n", opt, optarg);
    }

    if (optind >= argc)
    {
        return usage();
    }

    if (ZLog::IsDebug())
    {
        CreateFolder("./.zsign_debug");
        for (int i = optind; i < argc; i++)
        {
            ZLog::DebugV(">>> Argument:\t%s\n", argv[i]);
        }
    }

    string strPath = GetCanonicalizePath(argv[optind]);
    if (!IsFileExists(strPath.c_str()))
    {
        ZLog::ErrorV(">>> Invalid Path! %s\n", strPath.c_str());
        return -1;
    }

    bool bZipFile = false;
    if (!IsFolder(strPath.c_str()))
    {
        bZipFile = IsZipFile(strPath.c_str());
        if (!bZipFile)
        { //macho file
            ZMachO macho;
            if (macho.Init(strPath.c_str()))
            {
                if (!strDyLibFile.empty())
                { //inject dylib
                    bool bCreate = false;
                    macho.InjectDyLib(bWeakInject, strDyLibFile.c_str(), bCreate);
                }
                else
                {
                    macho.PrintInfo();
                }
                macho.Free();
            }
            return 0;
        }
    }

    ZTimer timer;
    ZSignAsset zSignAsset;
    if (!zSignAsset.Init(strCertFile, strPKeyFile, strProvFile, strEntitlementsFile, strPassword))
    {
        return -1;
    }

    bool bEnableCache = true;
    string strFolder = strPath;
    string inputFilePath;
    string outputDirPath;
    string unzipcmd;
    
    if (bZipFile)
    { //ipa file
        bForce = true;
        bEnableCache = false;

        // Generate folder name
        string tmpFolder;

        tmpFolder = zsignTmpPath + "zsign_folder_";

        StringFormat(strFolder, "%s%llu", tmpFolder.c_str(), timer.Reset());

        std::replace(strPath.begin(), strPath.end(), '\\', '/');
        std::replace(strFolder.begin(), strFolder.end(), '\\', '/');
        ZLog::PrintV(">>> Unzip:\t%s (%s) -> %s ... \n", strPath.c_str(), GetFileSizeString(strPath.c_str()).c_str(), strFolder.c_str());
        inputFilePath = strPath.c_str();
        outputDirPath = strFolder.c_str();

        string unzipcmd = "unzip -o \"" + inputFilePath + "\" -d \"" + outputDirPath + "\"";

        RemoveFolder(strFolder.c_str());

        int szstatus = system(unzipcmd.c_str());

        if (szstatus != 0)
        {
            RemoveFolder(strFolder.c_str());
            ZLog::ErrorV(">>> Unzip Failed!\n");
            return -1;
        }

        string attribcmd = "attrib -R \"" + outputDirPath + "/*\" /S /D";
        int attribstatus = system(attribcmd.c_str());

        timer.PrintResult(true, ">>> Unzip OK!");
    }

    timer.Reset();
    ZAppBundle bundle;
    bool bRet = bundle.SignFolder(&zSignAsset, strFolder, strBundleId, strBundleVersion, 
                                 strDisplayName, strDyLibFile, bForce, bWeakInject, 
                                 bEnableCache, excludeProvisioning, bInjectToFrameworks); // تمت إضافة bInjectToFrameworks
    timer.PrintResult(bRet, ">>> Signed %s!", bRet ? "OK" : "Failed");

    if (bInstall && strOutputFile.empty())
    {
        StringFormat(strOutputFile, "%szsign_temp_%llu.ipa", GetMicroSecond(), zsignTmpPath.c_str());
    }

    if (!strOutputFile.empty())
    {
        timer.Reset();
        size_t pos = bundle.m_strAppFolder.rfind("/Payload");
        if (string::npos == pos)
        {
            ZLog::Error(">>> Can't Find Payload Directory!\n");
            return -1;
        }

        ZLog::PrintV(">>> Archiving: \t%s ... \n", strOutputFile.c_str());
        string strBaseFolder = bundle.m_strAppFolder.substr(0, pos);
        char szOldFolder[PATH_MAX] = {0};
        if (NULL != getcwd(szOldFolder, PATH_MAX))
        {
            if (0 == chdir(strBaseFolder.c_str()))
            {
                uZipLevel = uZipLevel > 9 ? 9 : uZipLevel;
                std::replace(strOutputFile.begin(), strOutputFile.end(), '\\', '/');
                RemoveFile(strOutputFile.c_str());

                string zipcmd = "zip -r \"" + strOutputFile + "\" Payload";
                system(zipcmd.c_str());
                chdir(szOldFolder);
                if (!IsFileExists(strOutputFile.c_str()))
                {
                    ZLog::Error(">>> Archive Failed!\n");
                    return -1;
                }
            }
        }
        timer.PrintResult(true, ">>> Archive OK! (%s)", GetFileSizeString(strOutputFile.c_str()).c_str());
    }

    if (bRet && bInstall)
    {
        SystemExec("ideviceinstaller -i '%s'", strOutputFile.c_str());
    }
    
    string findPath;

    findPath = zsignTmpPath + "zsign_tmp_";
    if (0 == strOutputFile.find(findPath.c_str()))
    {
        RemoveFile(strOutputFile.c_str());
    }

    findPath = zsignTmpPath + "zsign_folder_";
    if (0 == strFolder.find(findPath.c_str()))
    {
        RemoveFolder(strFolder.c_str());
    }

    gtimer.Print(">>> Done.");
    return bRet ? 0 : -1;
}
