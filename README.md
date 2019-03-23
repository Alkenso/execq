#### execq
**execq** is a kind of ThreadPool idea implementation with extended features.
It supports different task sources and maintains task execution in parallel on N threads (according to hardware concurrency).
- maintains optimal thread count to avoid excess CPU context switch
- runs tasks from diffirent sources 'by turn', avoiding starvation for sources that adds task very late
- provides `queue-based` and `stream-based` approaches of task execution that covers different needs
- C++11 compilant


#### Queues and Streams
execq deals with concurrent task execution using two approaches: `queue-based` and `stream-based`

*You are free to use multimple queues and streams and any combinations of them!*

##### 1. Queue-based approach.
Designed to process some objects 'on demand', i.e. when some objects are available.

ExecutionQueue combines usual queue, synchronization mechanisms and parallel execution inside threadpool.

execq allows to create 'ExecutionQueue' object to process your objects there.
Now that is no need to write you own queue and synchronization around it - all is done inside!

    #include <execq/execq.h>
    
    // The function is called in parallel on the next free thread
    // with the next object from the queue.
    void ProcessObject(const std::atomic_bool& isCanceled, std::string object)
    {
        if (isCanceled)
        {
            std::cout << "Queue has been canceled. Skipping object...";
            return;
        }
        
        std::cout << "Processing object: " << object << '\n';
    }
        
    int main(void)
    {
        execq::ExecutionPool pool;
        
        std::unique_ptr<execq::IExecutionQueue<std::string>> queue = pool.createExecutionQueue<std::string>(&ProcessObject);
        
        queue->push("qwe");
        queue->push("some string");
        queue->push("");
        
        // Only for example purposes.
        // Usually here (if in 'main') could be RunLoop/EventLoop.
        // wait until objects are delivered to threads.
        sleep(5);
        
        return 0;
    }

##### 2. Stream-based approach.
Designed to process uncountable amount of tasks as fast as possible, i.e. process next task whenever new thread is available.

execq allows to create 'ExecutionStream' object that will execute your code each time the thread in the pool is ready to execute next task.
That approach should be considered as the most effective way to process unlimited (or almost unlimited) tasks.

    // The function is called each time the thread is ready to execute next task.
    // It is called only if stream is started.
    void ProcessNextObject(const std::atomic_bool& isCanceled)
    {
        if (isCanceled)
        {
            std::cout << "Stream has been canceled. Skipping...";
            return;
        }
        
        static std::atomic_int s_someObject { 0 };
        
        const int nextObject = s_someObject++;
        
        std::cout << "Processing object: " << nextObject << '\n';
    }

    int main(void)
    {
        execq::ExecutionPool pool;
        
        std::unique_ptr<execq::IExecutionStream> stream = pool.createExecutionStream(&ProcessNextObject);
        
        stream->start();
        
        // wait until some objects are processed. Only for example purposes.
        sleep(5);
        
        // Only for example purposes. Usually here (if in 'main') could be RunLoop/EventLoop.
        // Wait until some objects are processed.
        sleep(5);
        
        return 0;
    }

#### Design principles
Consider to use single ExecutionPool object (across whole application) with multiple queues and streams.
Combine queues and streams for free to achieve your goals
Be free to assign tasks to queue or operate stream even from the inside of it's callback.

#### Tech. details
##### 'by-turn' execution 
execq is designed in special way of dealing with the tasks of queues and streams to avoid starvation.

Let's assume simple example: there is 2 queues. 
First, 100 object are pushed to queue #1.
After 1 object is pushed to queue #2.

Now few tasks from queue #1 are being executed. But next task for execute will be the task from queue #2, and only then tasks from queue #1.

##### Avoiding queue starvation
Some tasks could be very time-comsumptive. That means they will block all pool threads execution for a long time.
This causes i.e. starvation: none of other queue tasks will be executed unless one of existing tasks is done.

To prevent this, each queue and stream additionally has it's own thread. This thread is some kind of 'insurance' thread, where the tasks from the queue/stream could be executed even if all pool's threads are busy for a long time.

#### Work to be done
- add cancellation of current tasks in the queue
- add cancellation of running tasks in the stream (when stopped)
