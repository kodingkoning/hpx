//  Copyright (c) 2008-2009 Chirag Dekate, Hartmut Kaiser, Anshul Tandon
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_THREAD_INIT_DATA_SEP_22_2009_1034AM)
#define HPX_THREAD_INIT_DATA_SEP_22_2009_1034AM

#include <hpx/hpx_fwd.hpp>
#include <hpx/runtime/naming/address.hpp>

#include <boost/function.hpp>

namespace hpx { namespace threads 
{
    ///////////////////////////////////////////////////////////////////////////
    struct thread_init_data
    {
        thread_init_data() 
          : description(0), lva(0), parent_prefix(0), parent_id(0), 
            parent_phase(0), priority(thread_priority_normal), 
            num_os_thread(std::size_t(-1))
        {}

        template <typename F>
        thread_init_data(F f, char const* desc = 0, 
                naming::address::address_type lva = 0,
                thread_priority priority = thread_priority_normal,
                std::size_t os_thread = std::size_t(-1)) 
          : func(f), description(desc), 
            lva(lva), parent_prefix(0), parent_id(0), parent_phase(0),
            priority(priority), num_os_thread(os_thread)
        {}

        boost::function<threads::thread_function_type> func;
        char const* description;
        naming::address::address_type lva;
        boost::uint32_t parent_prefix;
        threads::thread_id_type parent_id;
        std::size_t parent_phase;
        thread_priority priority;
        std::size_t num_os_thread;
    };
}}

#endif

