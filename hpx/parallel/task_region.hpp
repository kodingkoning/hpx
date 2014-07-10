//  Copyright (c) 2007-2014 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file task_region.hpp

#if !defined(HPX_PARALLEL_TASK_REGION_JUL_09_2014_1250PM)
#define HPX_PARALLEL_TASK_REGION_JUL_09_2014_1250PM

#include <hpx/hpx_fwd.hpp>
#include <hpx/exception.hpp>
#include <hpx/config/emulate_deleted.hpp>
#include <hpx/lcos/local/dataflow.hpp>
#include <hpx/lcos/local/spinlock.hpp>
#include <hpx/lcos/future.hpp>
#include <hpx/lcos/wait_all.hpp>
#include <hpx/async.hpp>

#include <hpx/parallel/config/inline_namespace.hpp>
#include <hpx/parallel/exception_list.hpp>
#include <hpx/parallel/execution_policy.hpp>

namespace hpx { namespace parallel { HPX_INLINE_NAMESPACE(v2)
{
    namespace detail
    {
        /// \cond NOINTERNAL
        ///////////////////////////////////////////////////////////////////////
        void handle_task_region_exceptions(parallel::exception_list& errors)
        {
            try {
                boost::rethrow_exception(boost::current_exception());
            }
            catch (parallel::exception_list const& el) {
                for (boost::exception_ptr e: el)
                    errors.add(e);
            }
            catch (...) {
                errors.add(boost::current_exception());
            }
        }
        /// \endcond
    }

    /// The class \a task_canceled_exception defines the type of objects thrown
    /// by task_region_handle::run or task_region_handle::wait if they detect
    /// that an exception is pending within the current parallel region.
    class task_canceled_exception : public hpx::exception
    {
    public:
        task_canceled_exception() BOOST_NOEXCEPT
          : hpx::exception(hpx::task_canceled_exception)
        {}
    };

    /// The class task_region_handle defines an interface for forking and
    /// joining parallel tasks. The \a task_region and \a task_region_final
    /// function templates create an object of type task_region_handle and
    /// pass a reference to that object to a user-provided callable object.
    ///
    /// An object of class \a task_region_handle cannot be constructed,
    /// destroyed, copied, or moved except by the implementation of the task
    /// region library. Taking the address of a task_region_handle object via
    /// operator& or addressof is ill formed. The result of obtaining its
    /// address by any other means is unspecified.
    ///
    /// A \a task_region_handle is active if it was created by the nearest
    /// enclosing task region, where "task region" refers to an invocation of
    /// task_region or task_region_final and "nearest enclosing" means the most
    /// recent invocation that has not yet completed. Code designated for
    /// execution in another thread by means other than the facilities in this
    /// section (e.g., using thread or async) are not enclosed in the task
    /// region and a task_region_handle passed to (or captured by) such code
    /// is not active within that code. Performing any operation on a
    /// task_region_handle that is not active results in undefined behavior.
    ///
    /// The \a task_region_handle that is active before a specific call to the
    /// run member function is not active within the asynchronous function
    /// that invoked run. (The invoked function should not, therefore, capture
    /// the \a task_region_handle from the surrounding block.)
    ///
    /// \code
    /// Example:
    ///     task_region([&](auto& tr) {
    ///         tr.run([&] {
    ///             tr.run([] { f(); });                // Error: tr is not active
    ///             task_region([&](auto& tr) {         // Nested task region
    ///                 tr.run(f);                      // OK: inner tr is active
    ///                 /// ...
    ///             });
    ///         });
    ///         /// ...
    ///     });
    /// \endcode
    ///
    class task_region_handle
    {
    private:
        /// \cond NOINTERNAL
        typedef hpx::lcos::local::spinlock mutex_type;

        template <typename F> friend void task_region(F &&);
        template <typename F> friend void task_region_final(F &&);

        template <typename F> friend
        hpx::future<void> task_region(task_execution_policy const&, F &&);
        template <typename F> friend
        hpx::future<void> task_region_final(task_execution_policy const&, F &&);

        task_region_handle()
        {
        }

        ~task_region_handle()
        {
            hpx::wait_all(when());
        }

        HPX_MOVABLE_BUT_NOT_COPYABLE(task_region_handle);
        BOOST_DELETED_FUNCTION(task_region_handle* operator&() const);

        // return future representing the execution of all tasks
        hpx::future<void> when(exception_list && errors)
        {
            std::vector<hpx::future<void> > active;

            {
                mutex_type::scoped_lock l(mtx_);
                std::swap(tasks_, active);
            }

            if (active.empty())
                return hpx::make_ready_future();

            return hpx::lcos::local::dataflow(
                [errors](std::vector<hpx::future<void> > && results) mutable
                {
                    for (hpx::future<void>& f: results)
                    {
                        if (f.has_exception())
                            errors.add(f.get_exception_ptr());
                    }
                    if (errors.get_error_count())
                        boost::throw_exception(errors);
                },
                std::move(active));
        }

        hpx::future<void> when()
        {
            return when(parallel::exception_list());
        }
        /// \endcond

    public:
        /// Causes the expression f() to be invoked asynchronously.
        /// The invocation of f is permitted to run on an unspecified thread
        /// in an unordered fashion relative to the sequence of operations
        /// following the call to run(f) (the continuation), or indeterminately
        /// sequenced within the same thread as the continuation.
        ///
        /// The call to \a run synchronizes with the invocation of f. The
        /// completion of f() synchronizes with the next invocation of wait on
        /// the same task_region_handle or completion of the nearest enclosing
        /// task region (i.e., the task_region or task_region_final that
        /// created this task_region_handle).
        ///
        /// Requires: F shall be MoveConstructible. The expression, (void)f(),
        ///           shall be well-formed.
        ///
        /// Precondition: this shall be the active task_region_handle.
        ///
        /// Postconditions: A call to run may return on a different thread than
        ///                 that on which it was called.
        ///
        /// \note The call to \a run is sequenced before the continuation as if
        ///       \a run returns on the same thread.
        ///       The invocation of the user-supplied callable object f may be
        ///       immediate or may be delayed until compute resources are
        ///       available. \a run might or might not return before invocation of
        ///       f completes.
        ///
        /// \throws \a task_canceled_exception, as described in Exception
        ///         Handling.
        ///
        template <typename F>
        void run(F && f)
        {
            hpx::future<void> result = hpx::async(boost::move(f));

            mutex_type::scoped_lock l(mtx_);
            tasks_.push_back(std::move(result));
        }

        /// Blocks until the tasks spawned using this task_region_handle have
        /// finished.
        ///
        /// Precondition: this shall be the active task_region_handle.
        ///
        /// Postcondition: All tasks spawned by the nearest enclosing task
        ///                region have finished. A call to wait may return on
        ///                a different thread than that on which it was called.
        ///
        /// \note The call to \a wait is sequenced before the continuation as if
        ///       \a wait returns on the same thread.
        ///
        /// \throws \a task_canceled_exception, as described in Exception
        ///         Handling.
        ///
        /// \code
        /// Example:
        ///     task_region([&](auto& tr) {
        ///         tr.run([&]{ process(a, w, x); }); // Process a[w] through a[x]
        ///         if (y < x) tr.wait();   // Wait if overlap between [w, x) and [y, z)
        ///         process(a, y, z);       // Process a[y] through a[z]
        ///     });
        /// \endcode
        ///
        void wait()
        {
            try {
                when().get();
            }
            catch (parallel::exception_list const&) {
                throw;
            }
            catch (...) {
                boost::throw_exception(
                    parallel::exception_list(boost::current_exception())
                );
            }
        }

    private:
        mutable mutex_type mtx_;
        std::vector<hpx::future<void> > tasks_;
    };

    /// Constructs a \a task_region_handle, tr, and invokes the expression
    /// \a f(tr) on the user-provided object, \a f.
    ///
    /// \tparam F   The type of the user defined function to invoke inside the
    ///             task_region (deduced). \a F shall be MoveConstructible.
    ///
    /// \param f    The user defined function to invoke inside the task_region.
    ///             Given an lvalue \a tr of type \a task_region_handle, the
    ///             expression, (void)f(tr), shall be well-formed.
    ///
    /// Postcondition: All tasks spawned from \a f have finished execution.
    ///                A call to task_region may return on a different thread
    ///                than that on which it was called.
    ///
    /// \throws An \a exception_list, as specified in Exception Handling.
    ///
    /// \note It is expected (but not mandated) that f will (directly or
    ///       indirectly) call tr.run(_callable_object_).
    ///
    template <typename F>
    void task_region(F && f)
    {
        task_region_handle trh;

        parallel::exception_list errors;
        try {
            f(trh);
        }
        catch (...) {
            detail::handle_task_region_exceptions(errors);
        }

        // regardless of whether f(trh) has thrown an exception we need to
        // obey the contract and wait for all tasks to join
        try {
            trh.wait();      // throws list of exceptions
        }
        catch (...) {
            detail::handle_task_region_exceptions(errors);
        }

        if (errors.get_error_count())
            boost::throw_exception(errors);
    }

    /// Constructs a \a task_region_handle, tr, and invokes the expression
    /// \a f(tr) on the user-provided object, \a f.
    ///
    /// \tparam F   The type of the user defined function to invoke inside the
    ///             task_region (deduced). \a F shall be MoveConstructible.
    ///
    /// \param f    The user defined function to invoke inside the task_region.
    ///             Given an lvalue \a tr of type \a task_region_handle, the
    ///             expression, (void)f(tr), shall be well-formed.
    ///
    /// \throws An \a exception_list, as specified in Exception Handling.
    ///
    /// Postcondition: All tasks spawned from \a f have finished execution.
    ///                A call to \a task_region_final always returns on the
    ///                same thread as that on which it was called.
    ///
    /// \note It is expected (but not mandated) that f will (directly or
    ///       indirectly) call tr.run(_callable_object_).
    ///
    template <typename F>
    void task_region_final(F && f)
    {
        // By design we always return on the same (HPX-) thread as we started
        // executing task_region_final.
        task_region(std::forward<F>(f));
    }

    /// Constructs a \a task_region_handle, tr, and invokes the expression
    /// \a f(tr) on the user-provided object, \a f.
    ///
    /// \tparam F   The type of the user defined function to invoke inside the
    ///             task_region (deduced). \a F shall be MoveConstructible.
    ///
    /// \param t    An instance of type task_execution_policy.
    /// \param f    The user defined function to invoke inside the task_region
    ///             Given an lvalue \a tr of type \a task_region_handle, the
    ///             expression, (void)f(tr), shall be well-formed.
    ///
    /// Postcondition: All tasks spawned from \a f have finished execution once
    ///                once the returned future has become ready.
    ///                A call to task_region may return on a different thread
    ///                than that on which it was called.
    ///
    /// \returns An instance of future<void> which will become ready once all
    ///          tasks spawned inside the \a task_region have finished
    ///          executing. Any exceptions thrown during execution of the
    ///          \a task_region or any of the spawned tasks are accessible
    ///          through the returned value as well.
    ///
    /// \throws An \a exception_list, as specified in Exception Handling.
    ///
    /// \note It is expected (but not mandated) that f will (directly or
    ///       indirectly) call tr.run(_callable_object_).
    ///
    template <typename F>
    hpx::future<void> task_region(task_execution_policy const& t, F && f)
    {
        task_region_handle trh;
        parallel::exception_list errors;

        try {
            f(trh);
        }
        catch (...) {
            detail::handle_task_region_exceptions(errors);
        }

        return trh.when(std::move(errors));
    }

    /// Constructs a \a task_region_handle, tr, and invokes the expression
    /// \a f(tr) on the user-provided object, \a f.
    ///
    /// \tparam F   The type of the user defined function to invoke inside the
    ///             task_region (deduced). \a F shall be MoveConstructible.
    ///
    /// \param t    An instance of type task_execution_policy.
    /// \param f    The user defined function to invoke inside the task_region.
    ///             Given an lvalue \a tr of type \a task_region_handle, the
    ///             expression, (void)f(tr), shall be well-formed.
    ///
    /// Postcondition: All tasks spawned from \a f have finished execution once
    ///                the returned future has become ready.
    ///                A call to \a task_region_final always returns on the
    ///                same thread as that on which it was called.
    ///
    /// \throws An \a exception_list, as specified in Exception Handling.
    ///
    /// \returns An instance of future<void> which will become ready once all
    ///          tasks spawned inside the \a task_region have finished
    ///          executing. Any exceptions thrown during execution of the
    ///          \a task_region or any of the spawned tasks are accessible
    ///          through the returned value as well.
    ///
    /// \note It is expected (but not mandated) that f will (directly or
    ///       indirectly) call tr.run(_callable_object_).
    ///
    template <typename F>
    hpx::future<void> task_region_final(task_execution_policy const& t, F && f)
    {
        // By design we always return on the same (HPX-) thread as we started
        // executing task_region_final.
        return task_region(task, std::forward<F>(f));
    }

#if !defined(BOOST_NO_CXX11_DELETED_FUNCTIONS)
    task_region_handle* addressof(task_region_handle&) = delete;
#endif
}}}

#endif
