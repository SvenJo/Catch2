/*
 *  Created by Martin on 25/07/2017
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <catch2/internal/catch_test_case_registry_impl.hpp>

#include <catch2/internal/catch_context.hpp>
#include <catch2/internal/catch_enforce.hpp>
#include <catch2/interfaces/catch_interfaces_registry_hub.hpp>
#include <catch2/internal/catch_random_number_generator.hpp>
#include <catch2/internal/catch_run_context.hpp>
#include <catch2/internal/catch_string_manip.hpp>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/catch_test_spec.hpp>

#include <algorithm>
#include <iterator>
#include <random>
#include <sstream>

namespace Catch {

namespace {
    struct HashTest {
        explicit HashTest(SimplePcg32& rng) {
            basis = rng();
            basis <<= 32;
            basis |= rng();
        }

        uint64_t basis;

        uint64_t operator()(TestCaseInfo const& t) const {
            // Modified FNV-1a hash
            static constexpr uint64_t prime = 1099511628211;
            uint64_t hash = basis;
            for (const char c : t.name) {
                hash ^= c;
                hash *= prime;
            }
            return hash;
        }
    };
} // end anonymous namespace

    std::vector<TestCaseHandle> sortTests( IConfig const& config, std::vector<TestCaseHandle> const& unsortedTestCases ) {
        switch (config.runOrder()) {
        case RunTests::InDeclarationOrder:
            return unsortedTestCases;

        case RunTests::InLexicographicalOrder: {
            std::vector<TestCaseHandle> sorted = unsortedTestCases;
            std::sort(sorted.begin(), sorted.end());
            return sorted;
        }
        case RunTests::InRandomOrder: {
            seedRng(config);
            HashTest h(rng());
            std::vector<std::pair<uint64_t, TestCaseHandle>> indexed_tests;
            indexed_tests.reserve(unsortedTestCases.size());

            for (auto const& handle : unsortedTestCases) {
                indexed_tests.emplace_back(h(handle.getTestCaseInfo()), handle);
            }

            std::sort(indexed_tests.begin(), indexed_tests.end());

            std::vector<TestCaseHandle> randomized;
            randomized.reserve(indexed_tests.size());

            for (auto const& indexed : indexed_tests) {
                randomized.push_back(indexed.second);
            }

            return randomized;
        }
        }

        CATCH_INTERNAL_ERROR("Unknown test order value!");
    }

    bool isThrowSafe( TestCaseHandle const& testCase, IConfig const& config ) {
        return !testCase.getTestCaseInfo().throws() || config.allowThrows();
    }

    bool matchTest( TestCaseHandle const& testCase, TestSpec const& testSpec, IConfig const& config ) {
        return testSpec.matches( testCase.getTestCaseInfo() ) && isThrowSafe( testCase, config );
    }

    void enforceNoDuplicateTestCases( std::vector<TestCaseHandle> const& functions ) {
        std::set<TestCaseHandle> seenFunctions;
        for( auto const& function : functions ) {
            auto prev = seenFunctions.insert( function );
            CATCH_ENFORCE( prev.second,
                    "error: TEST_CASE( \"" << function.getTestCaseInfo().name << "\" ) already defined.\n"
                    << "\tFirst seen at " << prev.first->getTestCaseInfo().lineInfo << "\n"
                    << "\tRedefined at " << function.getTestCaseInfo().lineInfo );
        }
    }

    std::vector<TestCaseHandle> filterTests( std::vector<TestCaseHandle> const& testCases, TestSpec const& testSpec, IConfig const& config ) {
        std::vector<TestCaseHandle> filtered;
        filtered.reserve( testCases.size() );
        for (auto const& testCase : testCases) {
            if ((!testSpec.hasFilters() && !testCase.getTestCaseInfo().isHidden()) ||
                (testSpec.hasFilters() && matchTest(testCase, testSpec, config))) {
                filtered.push_back(testCase);
            }
        }
        return filtered;
    }
    std::vector<TestCaseHandle> const& getAllTestCasesSorted( IConfig const& config ) {
        return getRegistryHub().getTestCaseRegistry().getAllTestsSorted( config );
    }

    void TestRegistry::registerTest(Detail::unique_ptr<TestCaseInfo> testInfo, Detail::unique_ptr<ITestInvoker> testInvoker) {
        m_handles.emplace_back(testInfo.get(), testInvoker.get());
        m_viewed_test_infos.push_back(testInfo.get());
        m_owned_test_infos.push_back(std::move(testInfo));
        m_invokers.push_back(std::move(testInvoker));
    }

    std::vector<TestCaseInfo*> const& TestRegistry::getAllInfos() const {
        return m_viewed_test_infos;
    }

    std::vector<TestCaseHandle> const& TestRegistry::getAllTests() const {
        return m_handles;
    }
    std::vector<TestCaseHandle> const& TestRegistry::getAllTestsSorted( IConfig const& config ) const {
        if( m_sortedFunctions.empty() )
            enforceNoDuplicateTestCases( m_handles );

        if(  m_currentSortOrder != config.runOrder() || m_sortedFunctions.empty() ) {
            m_sortedFunctions = sortTests( config, m_handles );
            m_currentSortOrder = config.runOrder();
        }
        return m_sortedFunctions;
    }



    ///////////////////////////////////////////////////////////////////////////
    void TestInvokerAsFunction::invoke() const {
        m_testAsFunction();
    }

    std::string extractClassName( StringRef const& classOrQualifiedMethodName ) {
        std::string className(classOrQualifiedMethodName);
        if( startsWith( className, '&' ) )
        {
            std::size_t lastColons = className.rfind( "::" );
            std::size_t penultimateColons = className.rfind( "::", lastColons-1 );
            if( penultimateColons == std::string::npos )
                penultimateColons = 1;
            className = className.substr( penultimateColons, lastColons-penultimateColons );
        }
        return className;
    }

} // end namespace Catch
