#include <iostream>
#include <cstdint>
#include <memory>
#include <vector>
#include <fstream>
#include <map>
#include <algorithm>
#include <iterator>

#undef ENABLE_DEBUG

//  Clang __unused-compat polyfill
#ifndef __unused
#define __unused
#endif

#ifdef ENABLE_DEBUG
#define DEBUG_STREAM std::cout
#else
std::ofstream null_stream("/dev/null");
#define DEBUG_STREAM null_stream
#endif

namespace thomas {
    class shop {
    private:
        uint64_t identifier;
        std::vector<shop *> connected_shops;

    public:
        explicit shop(const uint64_t identifier) : identifier(identifier) {
            //  Empty implementation
        }

        inline std::vector<shop *> &get_connected_shops() noexcept {
            return connected_shops;
        }

        inline const uint64_t get_identifier() const noexcept {
            return identifier;
        }
    };

    class network {
    private:
        struct _impact_entry {
            uint64_t impact;
        };

        std::map<uint64_t, thomas::shop *> shops;

        inline std::map<uint64_t, thomas::shop *> &get_shops() noexcept {
            return shops;
        }

    public:
        explicit network() {
            shops = std::map<uint64_t, thomas::shop *>();
        }

        virtual ~network() {
            for (auto &each_pair : shops) {
                delete each_pair.second;
            }

            shops.clear();
        }

        inline shop *shop_at(const uint64_t i) {
            return get_shops().at(i);
        }

        inline void register_shop(shop *shop) {
            shops.insert(std::make_pair(shop->get_identifier(), shop));
        }

        inline uint64_t number_of_connections() noexcept {
            uint64_t counter = 0;

            for (auto &each_pair : get_shops()) {
                shop *shop = each_pair.second;

                for (auto &remote_shop : shop->get_connected_shops()) {
                    (void)remote_shop;
                    counter += 1;
                }
            }

            return counter;
        }

        /**
         reduce()
         Reduces the network one step down by disposing nodes to two subtle parts.

         @return The number of nodes disposed.
         */
        inline uint64_t reduce() noexcept {
            std::vector<shop *> linear_shops;
            std::map<uint64_t, shop *> *shops = &get_shops();

            linear_shops.reserve(shops->size());

            std::for_each(shops->begin(),
                          shops->end(),
                          [&linear_shops] (const std::map<uint64_t, shop *>::value_type& pair) {
                linear_shops.push_back(pair.second);
            });

            //  Sort the linear container with number of connections
            std::sort(linear_shops.begin(),
                      linear_shops.end(),
                      [] (shop *lhs, shop *rhs) {
                return lhs->get_connected_shops().size() > rhs->get_connected_shops().size();
            });

            const uint64_t threshold = linear_shops.front()->get_connected_shops().size();

            DEBUG_STREAM << "reducing with threshold value of " << threshold << std::endl;

            //  Filter out the elements with connection size lower than threshold value
            linear_shops.erase(std::remove_if(linear_shops.begin(),
                                              linear_shops.end(),
                                              [&threshold] (shop *el) {
                return el->get_connected_shops().size() < threshold;
            }), linear_shops.end());

            std::vector<_impact_entry> impacts;
            impacts.reserve(linear_shops.size());

            //  Find the external impact of each node
            std::for_each(linear_shops.begin(),
                          linear_shops.end(),
                          [&impacts,
                           &linear_shops] (shop *el) {
                uint64_t impact = 0;

                for (auto &each_connection : el->get_connected_shops()) {
                    //  Check whether the connection is external
                    if (std::find(linear_shops.begin(), linear_shops.end(), each_connection) == linear_shops.end()) {
                        impact += each_connection->get_connected_shops().size();
                    }
                }

                impacts.push_back({
                    impact
                });
            });

            //  Sort the array with impact values descending
            std::sort(impacts.begin(),
                      impacts.end(),
                      [] (_impact_entry lhs,
                          _impact_entry rhs) {
                return lhs.impact > rhs.impact;
            });

            const uint64_t required_impact = impacts.front().impact;

            //  Filter out the elements with impact lower than required
            impacts.erase(std::remove_if(impacts.begin(),
                                         impacts.end(),
                                         [&required_impact /* clang-capture-default */] (_impact_entry el) {
                 return el.impact < required_impact;
             }), impacts.end());

            //  If the number of elements remaining is lower than two, no further action is required
            if (impacts.size() < 2) {
                return 0;
            }

            return impacts.size();
        }

        friend std::ostream &operator<<(std::ostream &os, network &network);
    };

    std::ostream &operator<< (std::ostream &os, network &network) {
        for (auto &each_pair : network.get_shops()) {
            shop *shop = each_pair.second;

            for (auto &remote_shop : shop->get_connected_shops()) {
                os << shop->get_identifier()  << " is connected with " << remote_shop->get_identifier() << std::endl;
            }
        }

        return os;
    }
}

int32_t main(int32_t argc, const char * argv[]) {
    if (argc != 2) {
        std::cerr << "argument error: missing file argument" << std::endl;
        std::terminate();
    }

    std::ifstream input_file(argv[1]);

    if (!input_file.is_open()) {
        std::cerr << "io error: file couldn't be opened" << std::endl;
        std::terminate();
    }

    std::string buffer_str;
    uint64_t num_shops, num_roads;

    //  Fetch the number of shops and roads
    std::getline(input_file, buffer_str);

    if (std::sscanf(buffer_str.data(), "%lu %lu", &num_shops, &num_roads) != 2) {
        std::cerr << "parsing error: couldn't parse header" << std::endl;
        std::terminate();
    }

    if (num_shops < 2 || num_shops > 1000) {
        std::cerr << "argument error: number of shops should be in between 2 to 1000 inclusive" << std::endl;
        std::terminate();
    }

    if (num_roads < 1 || num_roads > 1000) {
        std::cerr << "argument error: number of roads should be in between 1 to 1000 inclusive" << std::endl;
        std::terminate();
    }

    thomas::network network;

    for (uint64_t i = 0; i < num_roads && std::getline(input_file, buffer_str); ++i) {
        uint64_t shop_id, road_to;

        if (std::sscanf(buffer_str.data(), "%lu %lu", &shop_id, &road_to) != 2) {
            std::cerr << "parsing error: unexpected char stray - \"" << buffer_str << "\"" << std::endl;
            std::terminate();
        }

        if (shop_id < 1 || shop_id > 1000) {
            std::cerr << "warning: identifier for shop at line " << i + 2 << " is not in range 1 to 1000 inclusive: " << shop_id << std::endl;
            continue;
        }

        try {
            thomas::shop *shop = network.shop_at(shop_id);

            try {
                thomas::shop *dest = network.shop_at(road_to);

                dest->get_connected_shops().push_back(shop);
                shop->get_connected_shops().push_back(dest);
            } catch (std::out_of_range &oor2) {
                DEBUG_STREAM << "line " << i + 2 << ": destination shop with identifier " << road_to << " is being instantiated" << std::endl;

                thomas::shop *new_dest = new thomas::shop(road_to);

                new_dest->get_connected_shops().push_back(shop);
                shop->get_connected_shops().push_back(new_dest);

                network.register_shop(new_dest);
            }
        } catch (std::out_of_range &oor) {
            DEBUG_STREAM << "line " << i + 2 << ": source shop with identifier " << shop_id << " is being instantiated" << std::endl;

            thomas::shop *new_shop = new thomas::shop(shop_id);

            try {
                thomas::shop *dest = network.shop_at(road_to);

                dest->get_connected_shops().push_back(new_shop);
                new_shop->get_connected_shops().push_back(dest);
            } catch (std::out_of_range &oor2) {
                DEBUG_STREAM << "line " << i + 2 << ": destination shop with identifier " << road_to << " is being instantiated" << std::endl;

                thomas::shop *new_dest = new thomas::shop(road_to);

                new_dest->get_connected_shops().push_back(new_shop);
                new_shop->get_connected_shops().push_back(new_dest);

                network.register_shop(new_dest);
            }

            network.register_shop(new_shop);
        }
    }

    DEBUG_STREAM << network;
    DEBUG_STREAM << "network size: " << network.number_of_connections() << std::endl;

    std::cout << network.reduce() << std::endl;

    return 0;
}
