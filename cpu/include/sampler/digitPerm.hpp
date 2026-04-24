#pragma once
#include "utils/hash.hpp"
#include "utils/utility.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <memory.h>
#include <vector>

class DigitPermutation {
public:
  /// Build a set of per‑digit permutations for the given radix.
  explicit DigitPermutation(int base, std::uint32_t seed) : base_(base) {
    assert(base_ > 1 && "Base must be at least 2");

    // Figure out how many digits we ever need to examine
    // (same test PBRT uses to guarantee we stay < 1).
    double invBase = 1.0 / static_cast<double>(base_);
    double invBaseM = 1.0;
    while (1.0 - (base_ - 1) * invBaseM < 1.0) {
      ++nDigits_;
      invBaseM *= invBase;
    }

    // Allocate storage once, contiguous in memory.
    permutations_.resize(static_cast<std::size_t>(nDigits_) * base_);

    // Fill each digit‑level permutation table.
    for (int digitIndex = 0; digitIndex < nDigits_; ++digitIndex) {
      std::uint64_t dseed = gl::pbrt::hash(base_, digitIndex, seed);
      for (int digitValue = 0; digitValue < base_; ++digitValue) {
        std::size_t idx = static_cast<std::size_t>(digitIndex) * base_ +
                          static_cast<std::size_t>(digitValue);
        permutations_[idx] = gl::permutationElement(digitValue, base_, dseed);
      }
    }
  }

  /// Look up the permuted value for a digit in the Halton sequence.
  int permute(int digitIndex, int digitValue) const noexcept {
    std::size_t idx = static_cast<std::size_t>(digitIndex) * base_ +
                      static_cast<std::size_t>(digitValue);
    return permutations_[idx];
  }

  // Accessors if you need them.
  int base() const noexcept { return base_; }
  int nDigits() const noexcept { return nDigits_; }

private:
  int base_ = 0;
  int nDigits_ = 0;                         // number of digits we generated
  std::vector<std::uint16_t> permutations_; // contiguous permutation table
};

namespace gl {
static inline std::vector<DigitPermutation>
computeRadicalInversePermutations(std::uint32_t seed) {
  std::vector<DigitPermutation> perms;
  perms.reserve(static_cast<std::size_t>(PrimeCount));

  for (int i = 0; i < PrimeCount; ++i)
    perms.emplace_back(Primes[i], seed); // uses the std‑lib constructor

  return perms; // NRVO / move‑elision → no extra copy
}

inline double scrambledRadicalInverse(int baseIndex, std::uint64_t a,
                                      const DigitPermutation &perm) {
  const int base = Primes[baseIndex];
  const double invBase = 1.0 / static_cast<double>(base);
  double invBaseM = 1.0;

  std::uint64_t reversedDigits = 0;
  int digitIndex = 0;

  // Loop until the residual term we would add is < machine‑epsilon
  // (same termination test PBRT uses).
  while (1.0 - (base - 1) * invBaseM < 1.0) {
    std::uint64_t next = a / base;
    int digitValue = static_cast<int>(a - next * base);

    reversedDigits =
        reversedDigits * static_cast<std::uint64_t>(base) +
        static_cast<std::uint64_t>(perm.permute(digitIndex, digitValue));

    invBaseM *= invBase;
    ++digitIndex;
    a = next;
  }

  // Clamp to just below 1.0 to avoid ever returning 1.
  const double oneMinusEps = std::nextafter(1.0, 0.0);
  return std::min(invBaseM * static_cast<double>(reversedDigits), oneMinusEps);
}

inline double owenScrambledRadicalInverse(int baseIndex, std::uint64_t a,
                                          std::uint32_t hash) {
  const int base = Primes[baseIndex];
  const double invBase = 1.0 / static_cast<double>(base);
  double invBaseM = 1.0;

  std::uint64_t reversedDigits = 0;
  int digitIndex = 0;

  // Fixed termination condition to match other methods
  while (1.0 - (base - 1) * invBaseM < 1.0) {
    std::uint64_t next = a / base;
    int digitValue = static_cast<int>(a - next * base);

    // Improved hash mixing for Owen scrambling
    std::uint32_t digitHash =
        gl::pbrt::mixBits(hash ^ static_cast<std::uint32_t>(reversedDigits) ^
                          (static_cast<std::uint32_t>(digitIndex) << 16));

    digitValue = static_cast<int>(
        gl::permutationElement(static_cast<std::uint32_t>(digitValue),
                               static_cast<std::uint32_t>(base), digitHash));

    reversedDigits = reversedDigits * static_cast<std::uint64_t>(base) +
                     static_cast<std::uint64_t>(digitValue);

    invBaseM *= invBase;
    ++digitIndex;
    a = next;
  }

  // Clamp so we never return 1 exactly
  const double oneMinusEps = std::nextafter(1.0, 0.0);
  return std::min(invBaseM * static_cast<double>(reversedDigits), oneMinusEps);
}
}; // namespace gl