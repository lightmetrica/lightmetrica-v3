/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>
#include <thread>
#include <string>
#include <atomic>
#include <mutex>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <regex>
#include <tuple>
#include <numeric>
#include <condition_variable>
#ifdef __GNUC__
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif
#include <any>
#include <variant>
#include <type_traits>
#include <queue>
#include <format>

#define WIN32_LEAN_AND_MEAN
#include <cereal/cereal.hpp>
#include <cereal/archives/portable_binary.hpp>
#pragma warning(push)
#pragma warning(disable:4127)  // conditional expression is constant
#include <nlohmann/json.hpp>
#pragma warning(pop)