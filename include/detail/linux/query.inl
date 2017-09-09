#ifndef VMU_LINUX_QUERY_INL
#define VMU_LINUX_QUERY_INL

#include "../../query.hpp"
#include <fstream>
#include <sys/types.h>
#include <unistd.h>

namespace vmu {

    namespace detail 
    {
        
        template<typename Ptr>
        inline basic_region<Ptr> query_impl(std::ifstream& maps, Ptr address)
        {
            char prot[4];
            std::string str;
            Ptr prev_end;
            Ptr prev_begin;
            for (std::string str; maps; maps.ignore(INT32_MAX, '\n')) {
                std::getline(maps, str, '-');
                const auto begin_addr = stoull(str, nullptr, 16);

                if (begin_addr <= address) {
                    std::getline(maps, str, ' ');
                    const auto end_addr = stoull(str, nullptr, 16);

                    if (end_addr >= address) {
                        maps.read(prot, 4);

                        return remote_region{ begin_addr
                            , end_addr
                            , (prot[0] != '-') | ((prot[1] != '-') * 2) | ((prot[2] != '-') * 4)
                            , prot[3] != '-'
                            , true };

                    }
                    else
                        prev_end = end_addr;
                }
                else
                    prev_begin = begin_addr;
            }

            return remote_region{ prev_end
                , prev_begin
                , 0
                , false
                , false };
        }
    
        template<typename Ptr>
        inline std::vector<basic_region<Ptr>> query_range_impl(std::ifstream& maps, Ptr begin, Ptr end)
        {
            std::vector<remote_region> regions;
            char prot[4];
            for (std::string buf; maps && begin < end; maps.ignore(INT32_MAX, '\n')) {
                std::getline(maps, buf, '-');

                const auto begin_addr = stoull(buf, nullptr, 16);
                if (begin_addr < begin)
                    continue;

                std::getline(maps, buf, ' ');

                const auto end_addr = stoull(buf, nullptr, 16);
                begin = end_addr;

                maps.read(prot, 4);
                const protection::storage protection{ (prot[0] != '-')
                    | ((prot[1] != '-') * 2)
                    | ((prot[2] != '-') * 4) };

                // free memory
                if (!regions.empty() && regions.back().end != begin_addr)
                    regions.emplace_back(regions.back().end
                                         , begin_addr
                                         , protection::storage(0)
                                         , false
                                         , false);

                regions.emplace_back(begin_addr
                                     , end_addr
                                     , protection
                                     , prot[3] != '-'
                                     , true);
            }

            return regions;
        }

        std::ifstream open_maps(int pid)
        {
            std::ifstream maps("/proc/" + std::to_string(pid) + "/maps");
            if (!maps.is_open())
                throw std::runtime_error("failed to open proc/<pid>/maps");

            return maps;
        }

    }

    inline local_region query(std::uintptr_t address)
    {
        auto maps = detail::open_maps(::getpid());

        return detail::query_impl<std::uintptr_t>(maps, address);
    }
    inline local_region query(std::uintptr_t address, std::error_code& ec)
    {
        std::ifstream maps("/proc/" + std::to_string(::getpid()) + "/maps");
        if (!maps.is_open()) {
            ec = std::make_error_code(std::errc::io_error);
            return {};
        }

        return detail::query_impl<std::uintptr_t>(maps, address);
    }

    inline std::vector<local_region> query_range(std::uintptr_t begin, std::uintptr_t end)
    {
        auto maps = detail::open_maps(::getpid());

        return detail::query_range_impl<std::uintptr_t>(maps, begin, end);
    }
    inline std::vector<local_region> query_range(std::uintptr_t begin
                                           , std::uintptr_t end
                                           , std::error_code& ec)
    {
        std::ifstream maps("/proc/" + std::to_string(::getpid()) + "/maps");
        if (!maps.is_open()) {
            ec = std::make_error_code(std::errc::io_error);
            return {};
        }

        return detail::query_range_impl<std::uintptr_t>(maps, begin, end);
    }


    template<typename Handle>
    inline remote_region query(const Handle& handle, std::uint64_t address)
    {
        auto maps = detail::open_maps(handle);

        return detail::query_impl<std::uint64_t>(maps, address);
    }
    template<typename Handle>
    inline remote_region query(const Handle& handle, std::uint64_t address, std::error_code& ec)
    {
        std::ifstream maps("/proc/" + std::to_string(static_cast<int>(handle)) + "/maps");
        if (!maps.is_open()) {
            ec = std::make_error_code(std::errc::io_error);
            return {};
        }

        return detail::query_impl<std::uint64_t>(maps, address);
    }

    template<typename Handle>
    inline std::vector<remote_region> query_range(const Handle& handle
                                                  , std::uint64_t begin
                                                  , std::uint64_t end)
    {
        auto maps = detail::open_maps(handle);

        return detail::query_range_impl<std::uint64_t>(maps, begin, end);
    }
    template<typename Handle>
    inline std::vector<remote_region> query_range(const Handle& handle
                                                  , std::uint64_t begin
                                                  , std::uint64_t end
                                                  , std::error_code& ec)
    {
        std::ifstream maps("/proc/" + std::to_string(static_cast<int>(handle)) + "/maps");
        if (!maps.is_open()) {
            ec = std::make_error_code(std::errc::io_error);
            return {};
        }

        return detail::query_range_impl<std::uint64_t>(maps, begin, end);
    }
}

#endif // !VMU_LINUX_QUERY_INL