#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>


inline void file_exists(const std::string& file_path) {
    std::ifstream file {file_path};

    if (!file) {
        std::cerr << "Error occurred while reading " << file_path << std::endl;
        std::cerr << "Exiting..." << std::endl;
        exit(1);
    }
}


template<class T>
std::unique_ptr<T> read_dataset_file(const std::string& file_path) {
    // Since the copy constructor of std::istream is deleted,
    // we need to return a new object by pointer
    std::unique_ptr<T> file_ptr {new T {file_path.c_str()}};

    file_exists(file_path);

    return file_ptr;
}


class Timer {
private:
    // Type aliases to make accessing nested type easier
    using clock_t = std::chrono::high_resolution_clock;
    using millisecond_t = std::chrono::duration<double, std::milli>;

    std::chrono::time_point<clock_t> m_begin;
    std::string m_unit {" ms"};

public:
    Timer() {
        reset();
    }

    void reset() {
        m_begin = clock_t::now();
    }

    double elapsed() const {
        return std::chrono::duration_cast<millisecond_t>(clock_t::now() - m_begin).count();
    }

    const std::string& unit() { return m_unit; }
};


class Profiler {
private:
    using time_log_t = std::unordered_map<std::string, double>;
    using call_log_t = std::unordered_map<std::string, int>;

    static time_log_t& time_log() {
        static time_log_t _time_log;
        return _time_log;
    }

    static call_log_t& call_log() {
        static call_log_t _call_log;
        return _call_log;
    }

    std::string m_name;
    Timer m_timer;

public:
    explicit Profiler(std::string name) : m_name {std::move(name)}, m_timer() {};

    ~Profiler() {
        time_log()[m_name] += m_timer.elapsed();
        call_log()[m_name] += 1;
    }

    static void report() {
        std::cout << std::string(80, '-') << std::endl;

        for (const auto& kv: time_log()) {
            std::cout << "Function " << kv.first << ":" << std::endl;
            std::cout << "\tCalled: " << call_log()[kv.first] << " times" << std::endl;
            std::cout << "\tCPU time: " << kv.second << Timer().unit() << std::endl;
        }

        std::cout << std::string(80, '-') << std::endl;
    }

    static void clear() {
        time_log().clear();
        call_log().clear();
    }
};


class NotImplemented : public std::logic_error {
public:
    NotImplemented() : std::logic_error("Function not yet implemented") {};
};


// https://stackoverflow.com/questions/20834838/using-tuple-in-unordered-map
namespace std {
    namespace {
        // Code from boost
        // Reciprocal of the golden ratio helps spread entropy
        //     and handles duplicates.
        // See Mike Seymour in magic-numbers-in-boosthash-combine:
        //     https://stackoverflow.com/questions/4948780

        template<class T>
        inline void hash_combine(std::size_t& seed, T const& v) {
            seed ^= hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        // Recursive template code derived from Matthieu M.
        template<class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
        struct HashValueImpl {
            static void apply(size_t& seed, Tuple const& tuple) {
                HashValueImpl<Tuple, Index - 1>::apply(seed, tuple);
                hash_combine(seed, get<Index>(tuple));
            }
        };

        template<class Tuple>
        struct HashValueImpl<Tuple, 0> {
            static void apply(size_t& seed, Tuple const& tuple) {
                hash_combine(seed, get<0>(tuple));
            }
        };
    }

    template<class ... TT>
    struct hash<std::tuple<TT...>> {
        size_t operator()(std::tuple<TT...> const& tt) const {
            size_t seed = 0;
            HashValueImpl<std::tuple<TT...> >::apply(seed, tt);
            return seed;
        }

    };
}

#endif // UTILITIES_HPP
