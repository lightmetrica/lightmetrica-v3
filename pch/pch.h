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
#include <filesystem>
#include <any>
#include <variant>
#include <type_traits>
#include <queue>

#define WIN32_LEAN_AND_MEAN
#define FMT_HEADER_ONLY
#include <lm/ext/fmt/format.h>
#include <lm/ext/cereal/cereal.hpp>
#include <lm/ext/cereal/archives/portable_binary.hpp>