#ifndef THREAD_POOL_MYIDENTIFY_1985
#define THREAD_POOL_MYIDENTIFY_1985
  
#include <pthread.h>
#include <boost/function.hpp>
namespace threadpool
{
    typedef boost::function<void()> task_type;
#ifdef __cplusplus
    extern "C" {
#endif
    bool init_thread_pool(int thread_num = 0);
    void destroy_thread_pool();
    // flag = 1 indicate create a new thread to excute the task (useful for long run task).
    bool queue_work_task(task_type const& task,int flag);
    // reuse a named thread to excute the task, if not found, create one.
    bool queue_work_task_to_named_thread(task_type const& task, const std::string& threadname);
    bool get_named_thread(const std::string& threadname, pthread_t& pid);
    void terminate_named_thread(const std::string& threadname);
    //if failed,return 0. Otherwise return the unique id.
    int  queue_timer_task(task_type const& task,int delaysec,bool repeatflag);
    void deletetimerbyid(int timerid);
#ifdef __cplusplus
    }
#endif

}
#endif
