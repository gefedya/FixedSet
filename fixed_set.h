#ifndef FIXEDSETFINAL_FIXED_SET_H
#define FIXEDSETFINAL_FIXED_SET_H


#include <iostream>
#include <utility>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <random>
#include <optional>
#include <cassert>


using std::vector;
using std::pair;
using std::cin;
using std::cout;
using std::swap;
using std::endl;

const int64_t kBasicPrimeModulo = 2'000'000'009;
const int64_t kBasicRehashConstant = 4;

uint64_t SumOfSquares(const vector<int64_t>& vec) {
    uint64_t res = 0;
    for (const int64_t& el: vec) {
        res += el * el;
    }
    return res;
}


class HashFunction {
protected:
    int64_t prime_modulo;
    int64_t a;
    int64_t b;

public:
    HashFunction(): prime_modulo(kBasicPrimeModulo), a(1), b(1) {}

    HashFunction(int64_t a, int64_t b, int64_t prime = kBasicPrimeModulo):
            prime_modulo(prime), a(a), b(b)  {
        assert(a != 0);
    }

    int64_t operator()(int64_t num) const {
        int64_t res = (a * num + b) % prime_modulo;
        return res >= 0 ? res : res + prime_modulo; // residues are nonnegative
    }
};

bool IfHasCollisions(const HashFunction& hash, const vector<int64_t>& data,
                     size_t table_size) {
    vector<int64_t> if_present_table(table_size, 0);

    for (const int64_t& elem: data) {
        auto& cur = if_present_table[hash(elem) % table_size];
        if (cur > 0) {
            return true;
        }
        cur += 1;
    }
    return false;
}

vector<int64_t> BucketsDistribution(const HashFunction& hash,
                                    const std::vector<int64_t>& array, size_t table_size) {
    vector<int64_t> chains_lengths(table_size, 0);

    for (const int64_t& elem: array) {
        chains_lengths[hash(elem) % table_size] += 1;
    }
    return chains_lengths;
}

struct {
    bool operator()(const HashFunction& hash, const vector<int64_t>& data,
                                                    size_t table_size) const {
        return SumOfSquares(BucketsDistribution(hash, data, table_size)) >
                kBasicRehashConstant * data.size();
    }
} lin_hashtable_rehash_predicate;

struct {
    bool operator()(const HashFunction& hash, const vector<int64_t>& array,
                    size_t table_size) const {
        return IfHasCollisions(hash, array, table_size);
    }
} top_sq_hashtable_rehash_predicate;


template <class Predicate>
HashFunction GenerateHashWhilePredicate(const std::vector<int64_t>& data,
                                        size_t table_size,
                                        const Predicate& pred) {
    std::random_device device;
    std::mt19937 generator(device());
    std::uniform_int_distribution<int64_t> a_gen(1, kBasicPrimeModulo - 1);
    std::uniform_int_distribution<int64_t> b_gen(0, kBasicPrimeModulo - 1);
    HashFunction hash;
    do {
        hash = HashFunction(a_gen(generator), b_gen(generator));
    } while (pred(hash, data, table_size));
    return hash;
}


class HashTable {
protected:
    vector<std::optional<int64_t>> table_;
    HashFunction hash_;

public:
    HashTable() = default;

    template <class Predicate>
    void Initialize(const vector<int64_t>& data, size_t table_size, Predicate pred) {
        if (data.empty()) {
            table_ = {};
            hash_ = HashFunction();
            return;
        }

        hash_ = GenerateHashWhilePredicate(data, table_size, pred);
        table_ = vector<std::optional<int64_t>>(table_size);
        for (const int64_t& i : data) {
            table_[hash_(i) % table_size] = i;
        }
    }

    bool Contains(int64_t value) const {
        if (table_.empty()) {
            return false;
        }
        return table_[hash_(value) % table_.size()] == value;
    }

    vector<std::optional<int64_t>> GetTable() const {
        return table_;
    };

    HashFunction GetHash() const {
        return hash_;
    }
};


class FixedSet {
    std::vector<HashTable> top_tables_;
    HashFunction base_hash_;

public:
    FixedSet() = default;
    void Initialize(const vector<int>& elements);
    bool Contains(int value) const;
};

void FixedSet::Initialize(const vector<int> &elements) {
    if (elements.empty()) {
        // base_lin_tb_ = HashTable();
        base_hash_ = HashFunction();
        top_tables_ = {};
        return;
    }
    size_t sz = elements.size();
    std::vector<int64_t> elements_to_long(elements.begin(), elements.end());
    base_hash_ = GenerateHashWhilePredicate(elements_to_long,
                                            sz, lin_hashtable_rehash_predicate);
    std::vector<std::vector<int64_t>> base_table(sz);
    for (const auto& elem: elements_to_long) {
        base_table[base_hash_(elem) % sz].emplace_back(elem);
    }

    top_tables_ = {};
    top_tables_.reserve(base_table.size());
    for (const auto& bucket: base_table) {
        HashTable top_table;
        top_table.Initialize(bucket,
                             bucket.size() * bucket.size(), top_sq_hashtable_rehash_predicate);
        top_tables_.emplace_back(top_table);
    }
}


bool FixedSet::Contains(int value) const {
    auto val_long = static_cast<int64_t>(value);
    if (top_tables_.empty()) {
        return false;
    }

    size_t ind = base_hash_(val_long) % top_tables_.size();
    const HashTable& to_search = top_tables_[ind];
    if (to_search.GetTable().empty()) {
        return false;
    }
    return to_search.Contains(val_long);
}

#endif // FIXEDSETFINAL_FIXED_SET_H
