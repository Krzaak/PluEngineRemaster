//
// Created by Plutex on 4/27/26.
//

#ifndef PLUENGINE_PYTHONBUILDENVIRONMENT_H
#define PLUENGINE_PYTHONBUILDENVIRONMENT_H

#include <filesystem>
#include <cstdlib>
#include <string>

namespace fs = std::filesystem;

namespace Plu
{
    class PythonEnvironment {
        fs::path envPath;
        fs::path pythonBin;
        fs::path pipBin;

    public:
        PythonEnvironment() {
            // Ścieżka do venv silnika
            #ifdef _WIN32
                envPath = fs::path(std::getenv("APPDATA")) / "PluEngine" / "PythonEnv";
                pythonBin = envPath / "Scripts" / "python.exe";
                pipBin    = envPath / "Scripts" / "pip.exe";
            #else
                envPath = fs::path(std::getenv("HOME")) / ".local/share/PluEngine/PythonEnv";
                pythonBin = envPath / "bin" / "python";
                pipBin    = envPath / "bin" / "pip";
            #endif
        }

        // Sprawdź czy venv istnieje
        bool VenvExists() {
            return fs::exists(pythonBin);
        }

        // Utwórz venv
        bool CreateVenv() {
            std::string cmd = "python -m venv " + envPath.string();
            return std::system(cmd.c_str()) == 0;
        }

        // Sprawdź czy PyArmor jest zainstalowany w venvie
        bool PyarmorInstalled() {
#ifdef _WIN32
            fs::path pyarmorBin = envPath / "Scripts" / "pyarmor.exe";
#else
            fs::path pyarmorBin = envPath / "bin" / "pyarmor";
#endif

            return fs::exists(pyarmorBin);
        }


        // Zainstaluj PyArmor do venva
        bool InstallPyarmor() {
            std::string cmd = pipBin.string() + " install pyarmor";
            return std::system(cmd.c_str()) == 0;
        }

        // Uruchom obfuskację
        bool Obfuscate(const fs::path& scriptsDir, const fs::path& outputDir) {
#ifdef _WIN32
            fs::path pyarmorBin = envPath / "Scripts" / "pyarmor.exe";
#else
            fs::path pyarmorBin = envPath / "bin" / "pyarmor";
#endif

            std::string cmd = pyarmorBin.string()
                + " gen --output "
                + outputDir.string() + " "
                + scriptsDir.string();
            PLU_TRACE("Running command {}", cmd.c_str());
            return std::system(cmd.c_str()) == 0;
        }

        // Całość — setup + obfuskacja
        bool Setup() {
            if (!VenvExists()) {
                // Sprawdź czy w ogóle jest Python w systemie
                if (std::system("python --version >nul 2>&1") != 0) {
                    // Brak Pythona — poinformuj użytkownika
                    return false;
                }
                if (!CreateVenv()) return false;
            }

            if (!PyarmorInstalled()) {
                if (!InstallPyarmor()) return false;
            }

            return true;
        }
    };
}

#endif //PLUENGINE_PYTHONBUILDENVIRONMENT_H
