/**
 * @file       concepts.hpp
 * @brief      Generic C++20 concepts for the core domain layer
 * @standard   C++23
 */

#ifndef IOT_CORE_CONCEPTS_HPP
#define IOT_CORE_CONCEPTS_HPP

#include "common/error.hpp"
#include <concepts>

namespace iot::core
{
    /**
     * Minimal CRUD repository concept.
     *
     * Given a repository type `T`, an entity type `Entity`, and an id type
     * `Id`, checks that `T` exposes `GetById`, `Add`, and `Remove` with the
     * expected `Result<...>` return types. Used to statically verify that
     * mock and production repository implementations satisfy the same
     * contract as the `I*Repository` interfaces without requiring virtual
     * dispatch (e.g. for compile-time checks or non-polymorphic adapters).
     */
    template <typename T, typename Entity, typename Id>
    concept Repository = requires( T repo, Entity entity, Id id ) {
        { repo.GetById( id ) } -> std::same_as<Result<Entity>>;
        { repo.Add( entity ) } -> std::same_as<Result<void>>;
        { repo.Remove( id ) } -> std::same_as<Result<void>>;
    };

} // namespace iot::core

#endif // IOT_CORE_CONCEPTS_HPP
