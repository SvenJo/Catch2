/*
 *  Created by Phil on 4/4/2019.
 *  Copyright 2019 Two Blue Cubes Ltd. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef TWOBLUECUBES_CATCH_ENUMVALUESREGISTRY_H_INCLUDED
#define TWOBLUECUBES_CATCH_ENUMVALUESREGISTRY_H_INCLUDED

#include <catch2/interfaces/catch_interfaces_enum_values_registry.hpp>
#include <catch2/internal/catch_unique_ptr.hpp>

#include <vector>

namespace Catch {

    namespace Detail {

        Catch::Detail::unique_ptr<EnumInfo> makeEnumInfo( StringRef enumName, StringRef allValueNames, std::vector<int> const& values );

        class EnumValuesRegistry : public IMutableEnumValuesRegistry {

            std::vector<Catch::Detail::unique_ptr<EnumInfo>> m_enumInfos;

            EnumInfo const& registerEnum( StringRef enumName, StringRef allEnums, std::vector<int> const& values) override;
        };

        std::vector<StringRef> parseEnums( StringRef enums );

    } // Detail

} // Catch

#endif //TWOBLUECUBES_CATCH_ENUMVALUESREGISTRY_H_INCLUDED