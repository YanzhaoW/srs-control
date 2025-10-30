// #pragma once

// #include "srs/utils/CommonAlias.hpp"
// #include <boost/asio/any_io_executor.hpp>
// #include <boost/asio/this_coro.hpp>

// namespace srs::workflow
// {

//     class ParallelTask
//     {
//       public:
//         ParallelTask(asio::any_io_executor& io_executor)
//             : executor_{ io_executor }
//         {
//         }
//         asio::any_io_executor get_executor() { return executor_; }

//         auto operator()(auto ... tasks) -> auto
//         {
//             auto io_context = co_await asio::this_coro::executor;

//             for (auto* task : {&tasks ...})
//             {
//                 co_await task->async_resume()
//             }
//             co_await one_task.async_resume(true, asio::deferred);
//             co_await another_task.async_resume(true, asio::deferred);
//             auto input = co_yield true;
//             auto output = false;
//             while (true)
//             {
//                 auto one_task_res = one_task.async_resume(input, asio::deferred);
//                 auto another_task_res = another_task.async_resume(input, asio::deferred);
//                 auto group =
//                     asio::experimental::make_parallel_group(std::move(one_task_res), std::move(another_task_res));
//                 auto res = co_await group.async_wait(asio::experimental::wait_for_all(), asio::deferred);
//                 auto res1 = std::get<2>(res);
//                 auto res2 = std::get<4>(res);
//                 if (res1.has_value() and res2.has_value())
//                 {
//                     output = res1.value() and res2.value();
//                 }
//                 else
//                 {
//                     output = false;
//                 }
//                 input = co_yield output;
//             }
//             trace("sequential tasks returns");
//             co_return;
//         }

//       private:
//         asio::any_io_executor executor_;
//     };

// } // namespace srs::workflow
