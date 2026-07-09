#pragma once

#include <asio.hpp>
#include <asio/ip/basic_endpoint.hpp>
#include <asio/ip/udp.hpp>
#include <memory>
#include <type_traits>

namespace srs
{
    class App;
}

namespace srs::connection
{
    using udp = asio::ip::udp;
    using UDPEndpoint = asio::ip::basic_endpoint<asio::ip::udp>;

    class CommandBase;

    template <typename T>
    struct is_shared_ptr : std::false_type
    {
    };
    template <typename T>
    struct is_shared_ptr<std::shared_ptr<T>> : std::true_type
    {
    };

    template <typename T>
    concept SharedConnectionPtr = is_shared_ptr<T>::value and requires(T obj) { obj->get_name(); };

    template <typename T>
    struct IsConnectionType : std::false_type
    {
    };

    template <>
    struct IsConnectionType<CommandBase> : std::true_type
    {
    };

    // struct Info
    // {
    //     explicit Info(App* control_ptr)
    //         : control{ control_ptr }
    //     {
    //     }
    //     App* control = nullptr;
    //     int local_port_number = 0;
    // };

} // namespace srs::connection
