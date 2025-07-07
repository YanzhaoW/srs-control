// #include <array>
// #include <bitset>
// #include <numeric>
// #include <ranges>
// #include <srs/CommandHandlers.hpp>
// #include <srs/Control.hpp>
// #include <vector>
// #include <srs/CommonDefinitions.hpp>
// #include <fmt/format.h>

// namespace srs
// {
//     // TODO: why?
//     constexpr auto magic_data_size = 16;
//     constexpr auto INDEX_MAP_BIT_SIZE = 16;

//     template <typename T>
//     concept EntryTypeExact = std::same_as<T, EntryType>;

//     void acq_on_handler(App& control, DeviceIndex /*indices*/)
//     {
//         const auto data = std::vector<EntryType>{ 0, 15, 1 };
//         control.register_command<MessageMode::write>(data, NULL_ADDRESS);
//     }

//     void acq_off_handler(App& control, DeviceIndex /*indices*/)
//     {
//         const auto data = std::vector<EntryType>{ 0, 15, 0 };
//         control.register_command<MessageMode::write>(data, NULL_ADDRESS);
//     }

//     void reset_handler(App& control, DeviceIndex /*indices*/)
//     {
//         const auto data = std::vector<EntryType>{ 0, 0xffffffff, 0xffff0001 };
//         control.register_command<MessageMode::write>(data, NULL_ADDRESS);
//     }

//     void powercycle_hybrids_handler(App& control, DeviceIndex /*indices*/)
//     {
//         const auto data = std::vector<EntryType>{ 0, 0, 0x37f };
//         control.register_command<MessageMode::write>(data, I2C_ADDRESS);
//     }

//     void set_mask_handler(App& control, DeviceIndex /*indices*/)
//     {
//         const auto data = std::vector<EntryType>{ 0, 8, control.get_channel_address() };
//         control.register_command<MessageMode::write>(data);
//     }

//     void link_status_handler(App& control, DeviceIndex /*indices*/)
//     {
//         const auto data = std::vector<EntryType>{ 0, 16 };
//         control.register_command<MessageMode::write>(data);
//     }

//     inline auto make_reserved_vector(int size)
//     {
//         auto data = std::vector<EntryType>{};
//         data.reserve(size);
//         data.push_back(0);
//         return data;
//     }

//     inline void add_config_to_vector(auto& vec, EntryType pos, EntryTypeExact auto entry)
//     {
//         vec.push_back(pos);
//         vec.push_back(entry);
//     }

//     void system_register_handler(App& control, DeviceIndex /*indices*/)
//     {
//         auto data = make_reserved_vector(magic_data_size + 1);
//         std::iota(data.begin() + 1, data.end(), 0);

//         control.register_command<MessageMode::read>(data);
//     }

//     void trigger_acq_constants_handler(App& control, DeviceIndex /*indices*/)
//     {
//         const auto& config = control.get_fec_config();

//         constexpr auto data_size = 17;
//         auto data = make_reserved_vector(data_size);

//         // TODO: where do these magic numbers come from?

//         // NOLINTBEGIN (cppcoreguidelines-avoid-magic-numbers)
//         add_config_to_vector(data, 1, config.debug_data_format);
//         add_config_to_vector(data, 2, config.latency_reset);
//         add_config_to_vector(data, 3, config.latency_data_max);
//         add_config_to_vector(data, 4, config.latency_data_error);
//         add_config_to_vector(data, 9, config.tp_offset_first);
//         add_config_to_vector(data, 10, config.tp_offset);
//         add_config_to_vector(data, 11, config.tp_latency);
//         add_config_to_vector(data, 12, config.tp_number);
//         // NOLINTEND (cppcoreguidelines-avoid-magic-numbers)

//         control.register_command<MessageMode::write>(data);
//     }

//     void configure_hybrid_handler(App& control, DeviceIndex indices)
//     {
//         auto hybrid_map = std::bitset<INDEX_MAP_BIT_SIZE>{};
//         hybrid_map.set(indices.hybrid);

//         const auto& config = control.get_fec_config();

//         constexpr auto data_size = 7;
//         auto data = make_reserved_vector(data_size);

//         // TODO: Why 3 here?
//         if (config.clock_source == 3)
//         {
//             // TODO: this is not safe
//             const auto test_pulser_config = (config.tp_polarity << 7U) | (config.tp_width << 4U) | config.tp_skew;
//             const auto clock_bunch_counter_config = (config.ckbc_skew << 4U) | config.ckbc;
//             const auto clock_data_config = config.ckdt * 2;
//             // NOLINTBEGIN (cppcoreguidelines-avoid-magic-numbers)
//             add_config_to_vector(data, 2, test_pulser_config);
//             add_config_to_vector(data, 7, clock_bunch_counter_config);
//             add_config_to_vector(data, 5, clock_data_config);
//         }
//         else
//         {
//             add_config_to_vector(data, 2, config.tp_skew);
//             add_config_to_vector(data, 3, config.tp_width);
//             add_config_to_vector(data, 4, config.tp_polarity);
//             // NOLINTEND (cppcoreguidelines-avoid-magic-numbers)
//         }
//         control.register_command<MessageMode::write>(data, static_cast<uint16_t>(hybrid_map.to_ulong()));
//     }

//     const CommandsHandlerMap commands_handler_map{ { Commands::acq_on, acq_on_handler },
//                                                    { Commands::acq_off, acq_off_handler },
//                                                    { Commands::configure_hybrid, configure_hybrid_handler },
//                                                    { Commands::link_status, link_status_handler },
//                                                    { Commands::powercycle_hybrids, powercycle_hybrids_handler },
//                                                    // { Commands::read_adc,  } ,
//                                                    { Commands::reset, reset_handler },
//                                                    // { Commands::send_config,  } ,
//                                                    { Commands::set_mask, set_mask_handler },
//                                                    { Commands::system_registers, system_register_handler },
//                                                    { Commands::trigger_acq_constants, trigger_acq_constants_handler }

//     };

// } // namespace srs
