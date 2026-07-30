/**
 * @file       i_device_repository.hpp
 * @brief      Repository port for device persistence
 * @standard   C++23
 */

#ifndef IOT_CORE_I_DEVICE_REPOSITORY_HPP
#define IOT_CORE_I_DEVICE_REPOSITORY_HPP

#include "common/error.hpp"
#include "common/types.hpp"
#include <string>
#include <vector>

namespace iot::core
{
    /**
     * Abstract persistence boundary for DeviceInfo entities.
     *
     * Implementations (e.g. SqliteDeviceRepository) live in the database
     * module. This interface belongs to core so that use cases can depend
     * on it without depending on any concrete storage technology.
     */
    class IDeviceRepository
    {
        public:
        virtual ~IDeviceRepository() = default;

        [[nodiscard]] virtual Result<DeviceInfo> GetById( const std::string& deviceId ) = 0;

        [[nodiscard]] virtual Result<std::vector<DeviceInfo>> GetAll() = 0;

        [[nodiscard]] virtual Result<void> Add( const DeviceInfo& device ) = 0;

        [[nodiscard]] virtual Result<void> Update( const DeviceInfo& device ) = 0;

        [[nodiscard]] virtual Result<void> Remove( const std::string& deviceId ) = 0;
    };

} // namespace iot::core

#endif // IOT_CORE_I_DEVICE_REPOSITORY_HPP
