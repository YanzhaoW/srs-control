#include <boost/asio/ip/address.hpp>

#include <srs/workflow/Handler.hpp>
#include <srs/writers/BinaryFileWriter.hpp>
#include <srs/writers/JsonWriter.hpp>
#include <srs/writers/RootFileWriter.hpp>
#include <srs/writers/UDPWriter.hpp>

namespace srs::writer
{
    namespace
    {
        auto convert_str_to_endpoint(asio::thread_pool& thread_pool, std::string_view ip_port)
            -> std::optional<asio::ip::udp::endpoint>
        {
            const auto colon_pos = ip_port.find(':');
            if (colon_pos == std::string::npos)
            {
                spdlog::critical("Ill format socket string {:?}. Please set it as \"ip:port\"", ip_port);
                return {};
            }
            auto ip_string = ip_port.substr(0, colon_pos);
            auto port_str = ip_port.substr(colon_pos + 1);

            auto err_code = boost::system::error_code{};
            auto resolver = asio::ip::udp::resolver{ thread_pool };
            auto query = asio::ip::udp::resolver::query{ std::string{ ip_string }, std::string{ port_str } };
            auto iter = resolver.resolve(query, err_code);
            if (err_code)
            {
                spdlog::critical("Cannot query the ip address {:?}. Error code: {}", ip_port, err_code.message());
                return {};
            }
            return *iter;
        }

        auto check_root_dependency() -> bool
        {
#ifdef HAS_ROOT
            return true;
#else
            spdlog::error("Cannot output to a root file. Please make sure the program is "
                          "built with the ROOT library.");
            return false;
#endif
        }

    } // namespace

    Manager::Manager(workflow::Handler* processor)
        : workflow_handler_{ processor }
    {
    }

    Manager::~Manager() = default;

    auto Manager::is_convert_required(process::DataConvertOptions dependee) const -> bool
    {
        return std::ranges::any_of(
            convert_count_map_,
            [dependee](const auto& option_count) -> bool
            { return option_count.second > 0 && convert_option_has_dependency(dependee, option_count.first); });
    }

    void Manager::wait_for_finished() { boost::wait_for_all(write_futures_.begin(), write_futures_.end()); }

    auto Manager::add_binary_file(const std::string& filename, process::DataConvertOptions deser_mode) -> bool
    {
        auto& app = workflow_handler_->get_app();
        return binary_files_
            .try_emplace(filename, std::make_unique<BinaryFile>(app.get_io_context(), filename, deser_mode))
            .second;
    }

    auto Manager::add_udp_file(const std::string& filename, process::DataConvertOptions deser_mode) -> bool
    {
        auto& app = workflow_handler_->get_app();
        auto endpoint = convert_str_to_endpoint(app.get_io_context(), filename);
        if (endpoint.has_value())
        {
            return udp_files_.try_emplace(filename, std::make_unique<UDP>(app, std::move(endpoint.value()), deser_mode))
                .second;
        }
        return false;
    }

    // NOLINTNEXTLINE
    auto Manager::add_root_file(const std::string& filename) -> bool
    {
#ifdef HAS_ROOT
        auto& app = workflow_handler_->get_app();
        return root_files_
            .try_emplace(filename, std::make_unique<RootFile>(app.get_io_context(), filename.c_str(), "RECREATE"))
            .second;
#else
        return false;
#endif
    }

    auto Manager::add_json_file(const std::string& filename) -> bool
    {
        auto& app = workflow_handler_->get_app();
        return json_files_.try_emplace(filename, std::make_unique<Json>(app.get_io_context(), filename)).second;
    }

    void Manager::set_output_filenames(const std::vector<std::string>& filenames)
    {
        auto& app = workflow_handler_->get_app();

        for (const auto& filename : filenames)
        {
            const auto [filetype, convert_mode] = get_filetype_from_filename(filename);

            auto is_ok = false;

            switch (filetype)
            {
                case no_output:
                    spdlog::error("Extension of the filename {:?} cannot be recognized!", filename);
                    continue;
                    break;
                case bin:
                    is_ok = add_binary_file(filename, convert_mode);
                    break;
                case udp:
                    is_ok = add_udp_file(filename, convert_mode);
                    break;
                case root:
                    if (not check_root_dependency())
                    {
                        continue;
                    }
                    is_ok = add_root_file(filename);
                    break;
                case json:
                    is_ok = add_json_file(filename);
                    break;
            }

            if (is_ok)
            {
                spdlog::info("Add the output source {:?}", filename);
                ++(convert_count_map_.at(convert_mode));
            }
            else
            {
                spdlog::error("The filename {:?} has been already added!", filename);
            }
        }
    }
} // namespace srs::writer
