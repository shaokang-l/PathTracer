#pragma once
#include <type_traits>
enum class TransportMode { Radiance, Importance };
enum BxDFFlags {
  Reflection = 1 << 0,
  Transmission = 1 << 1,
  Diffuse = 1 << 2,
  Glossy = 1 << 3,
  Specular = 1 << 4,
  DiffuseReflection = Diffuse | Reflection,
  DiffuseTransmission = Diffuse | Transmission,
  GlossyReflection = Glossy | Reflection,
  GlossyTransmission = Glossy | Transmission,
  SpecularReflection = Specular | Reflection,
  SpecularTransmission = Specular | Transmission,
  All = ~0u
};

enum class BxDFReflTransFlags : uint32_t {
  Unset = 0,
  Reflection = 1u << 0,
  Transmission = 1u << 1,
  All = Reflection | Transmission
};

// Helper to get the underlying integer type
template <typename E> constexpr auto to_int(E e) noexcept {
  return static_cast<std::underlying_type_t<E>>(e);
}

// Bitwise OR
inline constexpr BxDFReflTransFlags operator|(BxDFReflTransFlags a,
                                              BxDFReflTransFlags b) noexcept {
  return static_cast<BxDFReflTransFlags>(to_int(a) | to_int(b));
}

// Bitwise AND
inline constexpr BxDFReflTransFlags operator&(BxDFReflTransFlags a,
                                              BxDFReflTransFlags b) noexcept {
  return static_cast<BxDFReflTransFlags>(to_int(a) & to_int(b));
}

// Bitwise XOR
inline constexpr BxDFReflTransFlags operator^(BxDFReflTransFlags a,
                                              BxDFReflTransFlags b) noexcept {
  return static_cast<BxDFReflTransFlags>(to_int(a) ^ to_int(b));
}

// Bitwise NOT
inline constexpr BxDFReflTransFlags operator~(BxDFReflTransFlags a) noexcept {
  return static_cast<BxDFReflTransFlags>(~to_int(a));
}

// Compound assignment OR
inline BxDFReflTransFlags &operator|=(BxDFReflTransFlags &a,
                                      BxDFReflTransFlags b) noexcept {
  return a = a | b;
}

// Compound assignment AND
inline BxDFReflTransFlags &operator&=(BxDFReflTransFlags &a,
                                      BxDFReflTransFlags b) noexcept {
  return a = a & b;
}

// Compound assignment XOR
inline BxDFReflTransFlags &operator^=(BxDFReflTransFlags &a,
                                      BxDFReflTransFlags b) noexcept {
  return a = a ^ b;
}

//! f == true, when there's no bit set. !!f == true when there's bit set
inline constexpr bool operator!(BxDFReflTransFlags f) noexcept {
  return to_int(f) == 0;
}