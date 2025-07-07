#pragma once

#include <boost/asio.hpp>

namespace srs
{
    class App;
}

namespace srs::connection
{
    using udp = boost::asio::ip::udp;

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
