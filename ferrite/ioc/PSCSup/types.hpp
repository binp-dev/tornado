#pragma once

#include <cstdint>
#include <type_traits>
#include <variant>

#include <core/panic.hpp>

#include <menuFtype.h>

using EpicsTypeVariant = std::variant<
    int8_t,
    uint8_t,
    int16_t,
    uint16_t,
    int32_t,
    uint32_t,
    int64_t,
    uint64_t,
    float,
    double
>;

template <typename T>
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
inline constexpr menuFtype epics_type_enum() {
    return EpicsTypeEnum<T>::value;
}

template <typename T>
struct EpicsEnumType;

template <template <typename, typename ...> typename Visitor, typename ...Args>
constexpr decltype(auto) visit_epics_enum(menuFtype enum_value, Args &&...args) {
    switch (enum_value) {
    case menuFtypeCHAR:
        return Visitor<int8_t>(std::forward(args)...);
    case menuFtypeUCHAR:
        return Visitor<uint8_t>(std::forward(args)...);
    case menuFtypeSHORT:
        return Visitor<int16_t>(std::forward(args)...);
    case menuFtypeUSHORT:
        return Visitor<uint16_t>(std::forward(args)...);
    case menuFtypeLONG:
        return Visitor<int32_t>(std::forward(args)...);
    case menuFtypeULONG:
        return Visitor<uint32_t>(std::forward(args)...);
    case menuFtypeINT64:
        return Visitor<int64_t>(std::forward(args)...);
    case menuFtypeUINT64:
        return Visitor<uint64_t>(std::forward(args)...);
    case menuFtypeFLOAT:
        return Visitor<float>(std::forward(args)...);
    case menuFtypeDOUBLE:
        return Visitor<double>(std::forward(args)...);
    case menuFtypeSTRING:
        panic("menuFtypeSTRING is not supported yet");
    case menuFtypeENUM:
        panic("menuFtypeENUM is not supported yet");
    }
    unreachable();
}
