#pragma once

#include <cstdint>
#include <type_traits>
#include <variant>

#include <core/panic.hpp>

#include <menuFtype.h>

template <typename>
struct EpicsTypeEnum;

template <> struct EpicsTypeEnum<int8_t> :
    std::integral_constant<menuFtype, menuFtypeCHAR> {};
template <> struct EpicsTypeEnum<uint8_t> :
    std::integral_constant<menuFtype, menuFtypeUCHAR> {};
template <> struct EpicsTypeEnum<int16_t> :
    std::integral_constant<menuFtype, menuFtypeSHORT> {};
template <> struct EpicsTypeEnum<uint16_t> :
    std::integral_constant<menuFtype, menuFtypeUSHORT> {};
template <> struct EpicsTypeEnum<int32_t> :
    std::integral_constant<menuFtype, menuFtypeLONG> {};
template <> struct EpicsTypeEnum<uint32_t> :
    std::integral_constant<menuFtype, menuFtypeULONG> {};
template <> struct EpicsTypeEnum<int64_t> :
    std::integral_constant<menuFtype, menuFtypeINT64> {};
template <> struct EpicsTypeEnum<uint64_t> :
    std::integral_constant<menuFtype, menuFtypeUINT64> {};
template <> struct EpicsTypeEnum<float> :
    std::integral_constant<menuFtype, menuFtypeFLOAT> {};
template <> struct EpicsTypeEnum<double> :
    std::integral_constant<menuFtype, menuFtypeDOUBLE> {};

template <typename T>
inline constexpr const menuFtype epics_type_enum = EpicsTypeEnum<T>::value;

template <typename T>
struct Phantom {
    using Type = T;
};

using EpicsTypeVariant = std::variant<
    Phantom<int8_t>,
    Phantom<uint8_t>,
    Phantom<int16_t>,
    Phantom<uint16_t>,
    Phantom<int32_t>,
    Phantom<uint32_t>,
    Phantom<int64_t>,
    Phantom<uint64_t>,
    Phantom<float>,
    Phantom<double>
>;

template <typename>
struct VariantsCount;

template <typename ...Types>
struct VariantsCount<std::variant<Types...>> :
    std::integral_constant<size_t, sizeof...(Types)> {};

template <typename V>
inline constexpr const size_t variants_count = VariantsCount<V>::value;

template <typename V>
struct TypeVariantTable;

template <typename ...Types>
struct TypeVariantTable<std::variant<Types...>> {
    static inline constexpr const EpicsTypeVariant value[] = { Types{}... };
};

using EpicsTypeVariantTable = TypeVariantTable<EpicsTypeVariant>;

inline constexpr const EpicsTypeVariant
    epics_type_variant_table[variants_count<EpicsTypeVariant>] = {};

template <typename V>
struct TypeEnumTable;

template <typename ...Args>
struct TypeEnumTable<std::variant<Args...>> {
    static inline constexpr const menuFtype value[] = { epics_type_enum<typename Args::Type>... };
};

using EpicsTypeEnumTable = TypeEnumTable<EpicsTypeVariant>;

inline EpicsTypeVariant epics_enum_type_variant(menuFtype enum_value) {
    for (size_t i = 0; i < variants_count<EpicsTypeVariant>; ++i) {
        if (EpicsTypeEnumTable::value[i] == enum_value) {
            return EpicsTypeVariantTable::value[i];
        }
    }
    unimplemented();
}
