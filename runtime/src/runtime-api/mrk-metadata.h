#pragma once

#include <cstdint>

#ifdef MRKLANG_COMPILER
#include <string>
#include <vector>
#include <memory>

#define UNIQUE std::true_type

namespace mrklang::runtime::metadata {
    // String type
    using String = std::string;

    // Container typedefs
    template<typename T, typename Unique = std::false_type>
    using Container = std::conditional_t<Unique::value, std::vector<std::unique_ptr<std::remove_pointer_t<T>>>, std::vector<T>>;
}
#else

#define UNIQUE std::nullptr_t

namespace mrklang::runtime::metadata {
    // String type
    using String = const char*;

    // Container typedefs - in runtime mode we just define a holder struct
    template<typename T, typename Unique = std::nullptr_t>
    struct Container {
        T* data;
        uint32_t count;

        T& operator[](size_t index) { return data[index]; }
        const T& operator[](size_t index) const { return data[index]; }

        uint32_t size() const { return count; }
    };
}
#endif

namespace mrklang::runtime::metadata {
    struct Type;

    struct Field {
        String name;
        Type* type;
    };

    struct Method {
        String name;
        Type* returnType;
        Container<Field*, UNIQUE> parameters;
    };

    struct Type {
        String name;
        Type* parent;
        Container<Field*, UNIQUE> fields;
        Container<Method*, UNIQUE> methods;

        struct {
            bool isPrimitive;       // Is this a fundamental type?
            bool isValue;           // Value type vs reference type
            bool isAbstract;        // Cannot be instantiated directly
            bool isSealed;          // Cannot be inherited from
            bool isGeneric;         // Is this a generic type?
            bool isEnum;            // Is this an enum?
            bool isInterface;       // Is this an interface?
			bool isNamespace;       // Is this a namespace?
            uint32_t size;          // Size in bytes of instances
            Container<Type*, UNIQUE> genericParameters;  // For generic types
        } traits;
    };

    struct Image {
        String name;
        Container<Type*, UNIQUE> types;
    };

    struct Metadata {
        Container<Image*, UNIQUE> images;
    };
}
