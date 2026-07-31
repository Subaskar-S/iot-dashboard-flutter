/**
 * @file       password_hasher_test.cpp
 * @standard   C++23
 */

#include "security/password_hasher.hpp"
#include <gtest/gtest.h>

namespace
{
    using namespace iot::security;

    TEST( PasswordHasherTest, HashProducesNonEmptyString )
    {
        PasswordHasher hasher( 1000 ); // low iterations for test speed
        auto result = hasher.Hash( "password123" );
        ASSERT_TRUE( result.has_value() );
        EXPECT_FALSE( result->empty() );
    }

    TEST( PasswordHasherTest, HashStartsWithExpectedPrefix )
    {
        PasswordHasher hasher( 1000 );
        auto result = hasher.Hash( "password" );
        ASSERT_TRUE( result.has_value() );
        EXPECT_TRUE( result->starts_with( "$pbkdf2-sha256$" ) );
    }

    TEST( PasswordHasherTest, TwoHashesOfSamePasswordDiffer )
    {
        PasswordHasher hasher( 1000 );
        auto h1 = hasher.Hash( "password" );
        auto h2 = hasher.Hash( "password" );
        ASSERT_TRUE( h1.has_value() && h2.has_value() );
        EXPECT_NE( h1.value(), h2.value() ); // different salts
    }

    TEST( PasswordHasherTest, VerifyCorrectPasswordReturnsTrue )
    {
        PasswordHasher hasher( 1000 );
        auto hash = hasher.Hash( "correct-horse-battery" );
        ASSERT_TRUE( hash.has_value() );
        EXPECT_TRUE( hasher.Verify( "correct-horse-battery", hash.value() ) );
    }

    TEST( PasswordHasherTest, VerifyWrongPasswordReturnsFalse )
    {
        PasswordHasher hasher( 1000 );
        auto hash = hasher.Hash( "correct-password" );
        ASSERT_TRUE( hash.has_value() );
        EXPECT_FALSE( hasher.Verify( "wrong-password", hash.value() ) );
    }

    TEST( PasswordHasherTest, VerifyEmptyHashReturnsFalse )
    {
        PasswordHasher hasher( 1000 );
        EXPECT_FALSE( hasher.Verify( "password", "" ) );
    }

    TEST( PasswordHasherTest, VerifyMalformedHashReturnsFalse )
    {
        PasswordHasher hasher( 1000 );
        EXPECT_FALSE( hasher.Verify( "password", "$not-our-format$abc" ) );
    }

} // namespace
