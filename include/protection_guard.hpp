#ifndef VMU_PROTECTION_GUARD_HPP
#define VMU_PROTECTION_GUARD_HPP

#include "query.hpp"
#include "protect.hpp"

namespace vmu {

    class protection_guard
    {
        struct old_prot_storage
        {
            std::uintptr_t      begin;
            std::uintptr_t      end;
            native_protection_t prot;
        };

        std::vector<old_prot_storage> _old;
    public:
        protection_guard(std::uintptr_t begin, std::uintptr_t end, protection::storage prot)
        {
            auto regions = query(begin, end);
            _old.reserve(regions.size());
            for (auto& region : regions) {
                if (!region)
                    throw std::runtime_error("attempt to protect free memory");

                _old.emplace_back(region.begin, region.end, region.prot);
            }

            protect(begin, end, prot);
        }

        template<typename Range>
        protection_guard(const Range& r, protection::storage prot)
            : protection_guard(r.begin, r.end, prot)
        {}

        protection_guard(std::uintptr_t begin
                         , std::uintptr_t end
                         , protection::storage prot
                         , std::error_code& ec)
        {
            auto regions = query(begin, end, ec);
            if (ec)
                return;

            _old.reserve(regions.size());
            for (auto& region : regions) {
                if (!region)
                    ec = std::make_error_code(std::errc::operation_not_permitted);

                _old.emplace_back(region.begin, region.end, region.prot);
            }

            protect(begin, end, prot);
        }

        template<typename Range>
        protection_guard(const Range& r, protection::storage prot, std::error_code& ec)
            : protection_guard(r.begin, r.end, prot, ec)
        {}

        /// \brief Restores the memory protection to its previous state
        ~protection_guard()
        {
            std::error_code ec;
            for (const auto& ele : _old)
                protect(ele.begin, ele.end, ele.prot, ec);
        }

        /// \brief not copy constructible
        protection_guard(const protection_guard&) = delete;
        /// \brief not copy assignable
        protection_guard& operator=(const protection_guard&) = delete;

        void restore()
        {
            while (!_old.empty()) {
                const auto& back = _old.back();
                protect(back.begin, back.end, back.prot);
                _old.pop_back();
            }
        }
        void restore(std::error_code& ec)
        {
            std::error_code ec;
            while (!_old.empty()) {
                const auto& back = _old.back();
                protect(back.begin, back.end, back.prot, ec);
                if (ec)
                    return;

                _old.pop_back();
            }
        }
    };

}

#endif // !VMU_PROTECTION_GUARD_HPP
