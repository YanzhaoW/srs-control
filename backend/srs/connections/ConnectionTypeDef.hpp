#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/ip/udp.hpp>
#include <memory>
#include <type_traits>

namespace srs
{
    class App;
}

namespace srs::connection
{
    using udp = boost::asio::ip::udp;
    using UDPEndpoint = boost::asio::ip::basic_endpoint<boost::asio::ip::udp>;

    template <int buffer_size>
    class Base;

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

    template <int buffer_size>
    struct IsConnectionType<Base<buffer_size>> : std::true_type
    {
    };

    struct Info
    {
        explicit Info(App* control_ptr)
            : control{ control_ptr }
        {
        }
        App* control = nullptr;
        int local_port_number = 0;
    };

} // namespace srs::connection
