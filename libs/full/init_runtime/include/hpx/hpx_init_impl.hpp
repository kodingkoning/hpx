//  Copyright (c)      2018 Mikael Simberg
//  Copyright (c) 2007-2016 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#define DEBUG(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
//#define DEBUG(...) do {} while(0) 

#include <hpx/assert.hpp>
#include <hpx/functional/bind_back.hpp>
#include <hpx/functional/function.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/hpx_main_winsocket.hpp>
#include <hpx/hpx_user_main_config.hpp>
#include <hpx/init_runtime/detail/run_or_start.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/prefix/find_prefix.hpp>
#include <hpx/runtime_configuration/runtime_mode.hpp>
#include <hpx/runtime_fwd.hpp>
#include <hpx/runtime_local/shutdown_function.hpp>
#include <hpx/runtime_local/startup_function.hpp>

#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#if defined(__FreeBSD__)
extern HPX_EXPORT char** freebsd_environ;
extern char** environ;
#endif

#if defined(HPX_WINDOWS) && defined(HPX_HAVE_APEX)
namespace apex {

    // force linking of the application with APEX
    HPX_SYMBOL_IMPORT std::string& version();
}    // namespace apex
#endif

namespace hpx {
    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is the main entry point for any HPX application. This function
    /// (or one of its overloads below) should be called from the users `main()`
    /// function. It will set up the HPX runtime environment and schedule the
    /// function given by \p f as a HPX thread.
    inline int init(
        util::function_nonser<int(hpx::program_options::variables_map&)> const&
            f,
        int argc, char** argv, init_params const& params)
    {
        DEBUG("init A 1");
#if defined(HPX_WINDOWS)
        detail::init_winsocket();
#if defined(HPX_HAVE_APEX)
        // artificially force the apex shared library to be loaded by the
        // application
        apex::version();
#endif
#endif
        DEBUG("init A 2");
        util::set_hpx_prefix(HPX_PREFIX);
#if defined(__FreeBSD__)
        freebsd_environ = environ;
#endif
        // set a handler for std::abort
        std::signal(SIGABRT, detail::on_abort);
        std::atexit(detail::on_exit);
        DEBUG("init A 3");
#if defined(HPX_HAVE_CXX11_STD_QUICK_EXIT)
        std::at_quick_exit(detail::on_exit);
#endif
        DEBUG("init A 4");
        return detail::run_or_start(f, argc, argv, params, true);
    }

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is the main entry point for any HPX application. This function
    /// (or one of its overloads below) should be called from the users `main()`
    /// function. It will set up the HPX runtime environment and schedule the
    /// function given by \p f as a HPX thread.
    inline int init(util::function_nonser<int(int, char**)> const& f, int argc,
        char** argv, init_params const& params)
    {
        DEBUG("init B");
        util::function_nonser<int(hpx::program_options::variables_map&)>
            main_f = util::bind_back(detail::init_helper, f);
        if (argc == 0 || argv == nullptr)
        {
            return init(main_f, detail::dummy_argc, detail::dummy_argv, params);
        }
        return init(main_f, argc, argv, params);
    }

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is the main entry point for any HPX application. This function
    /// (or one of its overloads below) should be called from the users `main()`
    /// function. It will set up the HPX runtime environment and schedule the
    /// function given by \p f as a HPX thread.
    inline int init(int argc, char** argv, init_params const& params)
    {
        DEBUG("init C");
        util::function_nonser<int(hpx::program_options::variables_map&)>
            main_f = static_cast<hpx_main_type>(::hpx_main);
        if (argc == 0 || argv == nullptr)
        {
            return init(main_f, detail::dummy_argc, detail::dummy_argv, params);
        }
        return init(main_f, argc, argv, params);
    }

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is the main entry point for any HPX application. This function
    /// (or one of its overloads below) should be called from the users `main()`
    /// function. It will set up the HPX runtime environment and schedule the
    /// function given by \p f as a HPX thread.
    inline int init(
        std::nullptr_t, int argc, char** argv, init_params const& params)
    {
        DEBUG("init D");
        util::function_nonser<int(hpx::program_options::variables_map&)> main_f;
        if (argc == 0 || argv == nullptr)
        {
            return init(main_f, detail::dummy_argc, detail::dummy_argv, params);
        }
        return init(main_f, argc, argv, params);
    }

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main entry point, which can be used to set up the
    /// runtime for an HPX application (the runtime system will be set up in
    /// console mode or worker mode depending on the command line settings).
    inline int init(init_params const& params)
    {
        DEBUG("init E");
        util::function_nonser<int(hpx::program_options::variables_map&)>
            main_f = static_cast<hpx_main_type>(::hpx_main);
        return init(main_f, detail::dummy_argc, detail::dummy_argv, params);
    }

#if defined(HPX_HAVE_INIT_START_OVERLOADS_COMPATIBILITY)
    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is the main entry point for any HPX application. This function
    /// (or one of its overloads below) should be called from the users `main()`
    /// function. It will set up the HPX runtime environment and schedule the
    /// function given by \p f as a HPX thread.
    inline int init(util::function_nonser<int(
                        hpx::program_options::variables_map& vm)> const& f,
        hpx::program_options::options_description const& desc_cmdline, int argc,
        char** argv, std::vector<std::string> const& cfg,
        startup_function_type startup, shutdown_function_type shutdown,
        hpx::runtime_mode mode)
    {
        DEBUG("init F");
        hpx::init_params iparams;
        iparams.desc_cmdline = desc_cmdline;
        iparams.cfg = cfg;
        iparams.startup = std::move(startup);
        iparams.shutdown = std::move(shutdown);
        iparams.mode = mode;
        return init(f, argc, argv, iparams);
    }

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is the main entry point for any HPX application. This function
    /// (or one of its overloads below) should be called from the users `main()`
    /// function. It will set up the HPX runtime environment and schedule the
    /// function given by \p f as a HPX thread.
    inline int init(int (*f)(hpx::program_options::variables_map& vm),
        hpx::program_options::options_description const& desc_cmdline, int argc,
        char** argv, startup_function_type startup,
        shutdown_function_type shutdown, hpx::runtime_mode mode)
    {
        DEBUG("init G");
        hpx::init_params iparams;
        iparams.desc_cmdline = desc_cmdline;
        iparams.startup = std::move(startup);
        iparams.shutdown = std::move(shutdown);
        iparams.mode = mode;
        return init(f, argc, argv, iparams);
    }

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main entry point, which can be used to set up the
    /// runtime for an HPX application (the runtime system will be set up in
    /// console mode or worker mode depending on the command line settings).
    ///
    /// In console mode it will execute the user supplied function `hpx_main`,
    /// in worker mode it will execute an empty `hpx_main`.
    inline int init(
        hpx::program_options::options_description const& desc_cmdline, int argc,
        char** argv, startup_function_type startup,
        shutdown_function_type shutdown, hpx::runtime_mode mode)
    {
        DEBUG("init H");
        hpx::init_params iparams;
        iparams.desc_cmdline = desc_cmdline;
        iparams.startup = std::move(startup);
        iparams.shutdown = std::move(shutdown);
        iparams.mode = mode;
        return init(argc, argv, iparams);
    }

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main entry point, which can be used to set up the
    /// runtime for an HPX application (the runtime system will be set up in
    /// console mode or worker mode depending on the command line settings).
    ///
    /// In console mode it will execute the user supplied function `hpx_main`,
    /// in worker mode it will execute an empty `hpx_main`.
    inline int init(
        hpx::program_options::options_description const& desc_cmdline, int argc,
        char** argv, std::vector<std::string> const& cfg,
        startup_function_type startup, shutdown_function_type shutdown,
        hpx::runtime_mode mode)
    {
        DEBUG("init I");
        hpx::init_params iparams;
        iparams.desc_cmdline = desc_cmdline;
        iparams.cfg = cfg;
        iparams.startup = std::move(startup);
        iparams.shutdown = std::move(shutdown);
        iparams.mode = mode;
        return init(argc, argv, iparams);
    }

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main entry point, which can be used to set up the
    /// runtime for an HPX application (the runtime system will be set up in
    /// console mode or worker mode depending on the command line settings).
    ///
    /// In console mode it will execute the user supplied function `hpx_main`,
    /// in worker mode it will execute an empty `hpx_main`.
    inline int init(int argc, char** argv, std::vector<std::string> const& cfg,
        hpx::runtime_mode mode)
    {
        DEBUG("init J");
        hpx::init_params iparams;
        iparams.cfg = cfg;
        iparams.mode = mode;
        return init(argc, argv, iparams);
    }

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main entry point, which can be used to set up the
    /// runtime for an HPX application (the runtime system will be set up in
    /// console mode or worker mode depending on the command line settings).
    ///
    /// In console mode it will execute the user supplied function `hpx_main`,
    /// in worker mode it will execute an empty `hpx_main`.
    inline int init(
        hpx::program_options::options_description const& desc_cmdline, int argc,
        char** argv, hpx::runtime_mode mode)
    {
        DEBUG("init K");
        hpx::init_params iparams;
        iparams.desc_cmdline = desc_cmdline;
        iparams.mode = mode;
        return init(argc, argv, iparams);
    }

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main entry point, which can be used to set up the
    /// runtime for an HPX application (the runtime system will be set up in
    /// console mode or worker mode depending on the command line settings).
    ///
    /// In console mode it will execute the user supplied function `hpx_main`,
    /// in worker mode it will execute an empty `hpx_main`.
    inline int init(
        hpx::program_options::options_description const& desc_cmdline, int argc,
        char** argv, std::vector<std::string> const& cfg,
        hpx::runtime_mode mode)
    {
        DEBUG("init L");
        hpx::init_params iparams;
        iparams.desc_cmdline = desc_cmdline;
        iparams.cfg = cfg;
        iparams.mode = mode;
        return init(argc, argv, iparams);
    }

    /// \fn int init(std::string const& app_name, int argc = 0, char** argv = nullptr)
    ///
    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main entry point, which can be used to set up the
    /// runtime for an HPX application (the runtime system will be set up in
    /// console mode or worker mode depending on the command line settings).
    inline int init(std::string const& app_name, int argc, char** argv,
        hpx::runtime_mode mode)
    {
        DEBUG("init M");
        using hpx::program_options::options_description;
        options_description desc =
            options_description("Usage: " + app_name + " [options]");
        hpx::init_params iparams;
        iparams.desc_cmdline = desc;
        iparams.mode = mode;
        return init(argc, argv, iparams);
    }

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main entry point, which can be used to set up the
    /// runtime for an HPX application (the runtime system will be set up in
    /// console mode or worker mode depending on the command line settings).
    inline int init(std::vector<std::string> const& cfg, hpx::runtime_mode mode)
    {
        DEBUG("init N");
        hpx::init_params iparams;
        iparams.cfg = cfg;
        iparams.mode = mode;
        return init(detail::dummy_argc, detail::dummy_argv, iparams);
    }

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main entry point, which can be used to set up the
    /// runtime for an HPX application (the runtime system will be set up in
    /// console mode or worker mode depending on the command line settings).
    inline int init(int (*f)(hpx::program_options::variables_map&),
        std::string const& app_name, int argc, char** argv,
        hpx::runtime_mode mode)
    {
        DEBUG("init O");
        using hpx::program_options::options_description;

        options_description desc_cmdline("Usage: " + app_name + " [options]");

        hpx::init_params iparams;
        iparams.desc_cmdline = desc_cmdline;
        iparams.mode = mode;

        if (argc == 0 || argv == nullptr)
        {
            return init(detail::dummy_argc, detail::dummy_argv, iparams);
        }

        return init(f, argc, argv, iparams);
    }

    // Main entry point for launching the HPX runtime system.
    inline int init(int (*f)(hpx::program_options::variables_map&), int argc,
        char** argv, hpx::runtime_mode mode)
    {
        DEBUG("init P");
        hpx::init_params iparams;

        if (argc == 0 || argv == nullptr)
        {
            return init(detail::dummy_argc, detail::dummy_argv, iparams);
        }

        iparams.mode = mode;
        return init(f, argc, argv, iparams);
    }

    /// \brief Main entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main entry point, which can be used to set up the
    /// runtime for an HPX application (the runtime system will be set up in
    /// console mode or worker mode depending on the command line settings).
    inline int init(util::function_nonser<int(int, char**)> const& f,
        std::string const& app_name, int argc, char** argv,
        hpx::runtime_mode mode)
    {
        DEBUG("init Q");
        using hpx::program_options::options_description;
        options_description desc_cmdline("Usage: " + app_name + " [options]");

        HPX_ASSERT(argc != 0 && argv != nullptr);

        hpx::init_params iparams;
        iparams.desc_cmdline = desc_cmdline;
        iparams.mode = mode;
        return init(f, argc, argv, iparams);
    }

    inline int init(util::function_nonser<int(int, char**)> const& f, int argc,
        char** argv, std::vector<std::string> const& cfg,
        hpx::runtime_mode mode)
    {
        DEBUG("init R");
        HPX_ASSERT(argc != 0 && argv != nullptr);

        hpx::init_params iparams;
        iparams.cfg = cfg;
        iparams.mode = mode;
        return init(f, argc, argv, iparams);
    }

    inline int init(util::function_nonser<int(int, char**)> const& f,
        std::vector<std::string> const& cfg, hpx::runtime_mode mode)
    {
        DEBUG("init S");
        hpx::init_params iparams;
        iparams.cfg = cfg;
        iparams.mode = mode;
        return init(f, detail::dummy_argc, detail::dummy_argv, iparams);
    }

    inline int init(std::nullptr_t, std::string const& app_name, int argc,
        char** argv, hpx::runtime_mode mode)
    {
        DEBUG("init T");
        using hpx::program_options::options_description;
        options_description desc_cmdline("Usage: " + app_name + " [options]");

        util::function_nonser<int(hpx::program_options::variables_map&)> main_f;

        HPX_ASSERT(argc != 0 && argv != nullptr);

        hpx::init_params iparams;
        iparams.desc_cmdline = desc_cmdline;
        iparams.mode = mode;
        return init(main_f, argc, argv, iparams);
    }

    inline int init(
        std::nullptr_t, int argc, char** argv, hpx::runtime_mode mode)
    {
        DEBUG("init U");
        HPX_ASSERT(argc != 0 && argv != nullptr);

        util::function_nonser<int(hpx::program_options::variables_map&)> main_f;

        hpx::init_params iparams;
        iparams.mode = mode;
        return init(main_f, argc, argv, iparams);
    }

    inline int init(std::nullptr_t, int argc, char** argv,
        std::vector<std::string> const& cfg, hpx::runtime_mode mode)
    {
        DEBUG("init V");
        HPX_ASSERT(argc != 0 && argv != nullptr);

        util::function_nonser<int(hpx::program_options::variables_map&)> main_f;

        hpx::init_params iparams;
        iparams.cfg = cfg;
        iparams.mode = mode;
        return init(main_f, argc, argv, iparams);
    }

    inline int init(std::nullptr_t, std::vector<std::string> const& cfg,
        hpx::runtime_mode mode)
    {
        DEBUG("init W");
        util::function_nonser<int(hpx::program_options::variables_map&)> main_f;
        hpx::init_params iparams;
        iparams.cfg = cfg;
        iparams.mode = mode;
        return init(main_f, detail::dummy_argc, detail::dummy_argv, iparams);
    }
#endif

}    // namespace hpx
