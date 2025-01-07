#pragma once

#include "pch.h"

namespace DirectoryManager {

    std::filesystem::path getUserDocumentsPath(std::initializer_list<std::string> endings = {}) {
        static char userProfile[128]; // Static char array to hold the environment variable
        char* buffer = nullptr;
        size_t bufferSize = 0;

        // Retrieve the USERPROFILE environment variable securely
        if (_dupenv_s(&buffer, &bufferSize, "USERPROFILE") == 0 && buffer != nullptr) {
            if (bufferSize >= sizeof(userProfile)) {
                free(buffer); // Free the buffer before throwing
                throw std::runtime_error("Environment variable value is too large for the static buffer.");
            }

            // Use strncpy_s for safe copying
            errno_t err = strncpy_s(userProfile, sizeof(userProfile), buffer, bufferSize - 1);
            if (err != 0) {
                free(buffer); // Free the buffer before throwing
                throw std::runtime_error("Error copying environment variable value.");
            }

            userProfile[sizeof(userProfile) - 1] = '\0'; // Ensure null termination
            free(buffer); // Free the dynamically allocated buffer

            // Start with the user profile path
            std::filesystem::path finalPath(userProfile);

            // Add the additional path segments (endings)
            for (const auto& ending : endings) {
                finalPath /= ending;
            }

            // Convert finalPath to string and copy into the static char array
            static char projectLocation[128]; // Static char array to hold the result path
            std::string pathString = finalPath.string(); // Convert the path to a string

            // Use strncpy_s to copy the string into the static char array safely
            errno_t errCopy = strncpy_s(projectLocation, sizeof(projectLocation), pathString.c_str(), pathString.length());
            if (errCopy != 0) {
                throw std::runtime_error("Error copying the final path into projectLocation.");
            }

            return finalPath; // Return the std::filesystem::path (if needed)
        }

        throw std::runtime_error("USERPROFILE environment variable not found.");
    }

}