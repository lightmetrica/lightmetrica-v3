/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/component.h>
#include <lm/logger.h>

#if LM_PLATFORM_WINDOWS
#include <Windows.h>
#elif LM_PLATFORM_LINUX || LM_PLATFORM_APPLE
#include <dlfcn.h>
#endif

LM_NAMESPACE_BEGIN(LM_NAMESPACE::comp)

// ----------------------------------------------------------------------------

// Platform-independent abstruction of shared library.
class SharedLibrary {
public:

    // Load library.
    bool load(const std::string& path) {
        #if LM_PLATFORM_WINDOWS
        const auto p = path + ".dll";
        #elif LM_PLATFORM_LINUX
        const auto p = path + ".so";
        #elif LM_PLATFORM_APPLE
        const auto p = path + ".dylib";
        #endif

        #if LM_PLATFORM_WINDOWS
        handle = LoadLibraryA(p.c_str());
        if (!handle) {
            std::cerr << fmt::format("Failed to load library or its dependencies: {}", p) << std::endl;
            std::cerr << getLastErrorAsString() << std::endl;
            return false;
        }
        #elif LM_PLATFORM_LINUX || LM_PLATFORM_APPLE
        handle = dlopen(p.c_str(), RTLD_LAZY | RTLD_LOCAL);
        if (!handle) {
            std::cerr << fmt::format("Failed to load library or its dependencies: {}", p) << std::endl;
            std::cerr << dlerror() << std::endl;
            return false;
        }
        #endif

        return true;
    }

    // Unload library.
    bool unload() {
        #if LM_PLATFORM_WINDOWS
        if (!FreeLibrary(handle)) {
            std::cerr << "Failed to free library" << std::endl;
            std::cerr << getLastErrorAsString() << std::endl;
            return false;
        }
        #elif LM_PLATFORM_LINUX || LM_PLATFORM_APPLE
        if (dlclose(handle) != 0) {
            std::cerr << "Failed to free library" << std::endl;
            std::cerr << dlerror() << std::endl;
            return false;
        }
        #endif

        return true;
    }

    // Retrieve an address of an exported symbol.
    void* getFuncPointer(const std::string& symbol) const {
        #if LM_PLATFORM_WINDOWS
        void* address = (void*)GetProcAddress(handle, symbol.c_str());
        if (address == nullptr) {
            std::cerr << fmt::format("Failed to get address of '{}'", symbol) << std::endl;
            std::cerr << getLastErrorAsString() << std::endl;
            return nullptr;
        }
        #elif LM_PLATFORM_LINUX || LM_PLATFORM_APPLE
        void* address = dlsym(handle, symbol.c_str());
        if (address == nullptr) {
            std::cerr << fmt::format("Failed to get address of '{}'", symbol) << std::endl;
            std::cerr << dlerror() << std::endl;
            return nullptr;
        }
        #endif

        return address;
    }

private:

    #if LM_PLATFORM_WINDOWS
    auto getLastErrorAsString() const -> std::string {
        DWORD error = GetLastError();
        if (error == 0) {
            return std::string();
        }

        LPSTR buffer = nullptr;
        size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&buffer, 0, NULL);
        std::string message(buffer, size);
        LocalFree(buffer);

        return message;
    }
    #endif

private:
    
    #if LM_PLATFORM_WINDOWS
	HMODULE handle;
    #elif LM_PLATFORM_LINUX || LM_PLATFORM_APPLE
    void* handle;
    #endif

};

// ----------------------------------------------------------------------------

class Impl_ {
public:

    static Impl_& instance() {
        static Impl_ instance;
        return instance;
    }

public:

    Component* createComp(const char* key) {
        auto it = funcMap_.find(key);
        if (it == funcMap_.end()) {
            return nullptr;
        }
        auto* p = it->second.createFunc();
        p->key_ = key;
        p->createFunc_ = it->second.createFunc;
        p->releaseFunc_ = it->second.releaseFunc;
        return p;
    }

    void reg(
        const char* key,
        const Component::CreateFunction& createFunc,
        const Component::ReleaseFunction& releaseFunc) {
        if (funcMap_.find(key) != funcMap_.end()) {
            std::cerr << fmt::format("Failed to register [ {} ]. Already registered.", key) << std::endl;
        }
        funcMap_[key] = CreateAndReleaseFunctions{ createFunc, releaseFunc };
    }

    void unreg(const char* key) {
        funcMap_.erase(key);
    }

    Component::ReleaseFunction releaseFunc(const char* key) {
        auto it = funcMap_.find(key);
        return it == funcMap_.end() ? nullptr : it->second.releaseFunc;
    }

    bool loadPlugin(const char* p) {
        namespace fs = std::filesystem;

        fs::path path(p);

        LM_LOG_INFO("Loading '{}'", path.filename().string());
        LM_LOG_INDENTER();

        // Load plugin
        std::unique_ptr<SharedLibrary> plugin(new SharedLibrary);
        #if LM_PLATFORM_WINDOWS
        const auto parent = path.parent_path().string();
        SetDllDirectory(parent.c_str());
        #endif
        if (!plugin->load(path.string())) {
            LM_LOG_WARN("Failed to load library: {}", path.string());
            return false;
        }
        #if LM_PLATFORM_WINDOWS
        SetDllDirectory(nullptr);
        #endif

        plugins_.push_back(std::move(plugin));
        LM_LOG_INFO("Successfully loaded");
        return true;
    }

    void loadPlugins(const char* directory) {
        namespace fs = std::filesystem;

        // Skip if directory does not exist
        if (!fs::is_directory(fs::path(directory))) {
            LM_LOG_WARN("Missing plugin directory '{}'. Skipping.", directory);
            return;
        }

        // File format
        #if LM_PLATFORM_WINDOWS
        const std::regex pluginNameExp("([0-9a-z_]+)\\.dll$");
        #elif LM_PLATFORM_LINUX
        const std::regex pluginNameExp("^([0-9a-z_]+)\\.so$");
        #elif LM_PLATFORM_APPLE
        const std::regex pluginNameExp("^([0-9a-z_]+)\\.dylib$");
        #endif

        // Enumerate dynamic libraries in #pluginDir
        fs::directory_iterator endIter;
        for (fs::directory_iterator it(directory); it != endIter; ++it) {
            if (!fs::is_regular_file(it->status())) {
                continue;
            }
            std::cmatch match;
            auto filename = it->path().filename().string();
            if (!std::regex_match(filename.c_str(), match, pluginNameExp)) {
                continue;
            }
            if (!loadPlugin(it->path().stem().string().c_str())) {
                continue;
            }
        }
    }

    void unloadPlugins() {
        for (auto& plugin : plugins_) { plugin->unload(); }
        plugins_.clear();
    }

private:

    // Registered implementations
    struct CreateAndReleaseFunctions
    {
        Component::CreateFunction createFunc;
        Component::ReleaseFunction releaseFunc;
    };
    std::unordered_map<std::string, CreateAndReleaseFunctions> funcMap_;

    // Loaded plugins
    std::vector<std::unique_ptr<SharedLibrary>> plugins_;

};

// ----------------------------------------------------------------------------

LM_PUBLIC_API Component* detail::createComp(const char* key) {
    return Impl_::instance().createComp(key);
}

LM_PUBLIC_API void detail::reg(
    const char* key,
    const Component::CreateFunction& createFunc,
    const Component::ReleaseFunction& releaseFunc) {
    Impl_::instance().reg(key, createFunc, releaseFunc);
}

LM_PUBLIC_API void detail::unreg(const char* key) {
    Impl_::instance().unreg(key);
}

LM_PUBLIC_API Component::ReleaseFunction detail::releaseFunc(const char* key) {
    return Impl_::instance().releaseFunc(key);
}

LM_PUBLIC_API bool detail::loadPlugin(const char* path) {
    return Impl_::instance().loadPlugin(path);
}

LM_PUBLIC_API void detail::loadPlugins(const char* directory) {
    Impl_::instance().loadPlugins(directory);
}

LM_PUBLIC_API void detail::unloadPlugins() {
    Impl_::instance().unloadPlugins();
}

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE::comp)
