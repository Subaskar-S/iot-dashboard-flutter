/**
 * @file       concepts_test.cpp
 * @brief      Unit tests for iot::core concepts
 * @standard   C++23
 */

#include "core/concepts.hpp"
#include <gtest/gtest.h>
#include <string>
#include <unordered_map>

namespace
{
    // Minimal in-memory fake satisfying iot::core::Repository<T, Entity, Id>.
    // Used only to verify the concept accepts a conforming type at compile
    // time; correctness of any real repository is tested where it's
    // implemented (e.g. the database module).
    class FakeIntRepository
    {
        public:
        [[nodiscard]] iot::Result<int> GetById( const std::string& id )
        {
            auto it = m_store.find( id );
            if ( it == m_store.end() )
            {
                return std::unexpected( iot::Error::DataNotFound );
            }
            return it->second;
        }

        [[nodiscard]] iot::Result<void> Add( int entity )
        {
            m_store[std::to_string( entity )] = entity;
            return {};
        }

        [[nodiscard]] iot::Result<void> Remove( const std::string& id )
        {
            m_store.erase( id );
            return {};
        }

        private:
        std::unordered_map<std::string, int> m_store;
    };

    // A type that looks similar but does NOT satisfy the concept (wrong
    // return type on GetById) to confirm the concept actually constrains.
    class NonConformingRepository
    {
        public:
        [[nodiscard]] int GetById( const std::string& )
        {
            return 0;
        }
        [[nodiscard]] iot::Result<void> Add( int )
        {
            return {};
        }
        [[nodiscard]] iot::Result<void> Remove( const std::string& )
        {
            return {};
        }
    };

    static_assert( iot::core::Repository<FakeIntRepository, int, std::string> );
    static_assert( !iot::core::Repository<NonConformingRepository, int, std::string> );

} // namespace

TEST( RepositoryConceptTest, ConformingTypeSupportsCrudOperations )
{
    FakeIntRepository repo;

    auto addResult = repo.Add( 42 );
    ASSERT_TRUE( addResult.has_value() );

    auto getResult = repo.GetById( "42" );
    ASSERT_TRUE( getResult.has_value() );
    EXPECT_EQ( getResult.value(), 42 );

    auto removeResult = repo.Remove( "42" );
    ASSERT_TRUE( removeResult.has_value() );

    auto missingResult = repo.GetById( "42" );
    ASSERT_FALSE( missingResult.has_value() );
    EXPECT_EQ( missingResult.error(), iot::Error::DataNotFound );
}
