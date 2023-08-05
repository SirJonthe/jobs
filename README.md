# jobs
## Copyright
Public domain, 2023

github.com/SirJonthe

## About
`jobs` is a minimalist library that introduces a the concept of jobs, or tasks, to C++11.

A job, or a task, within the context of this library, is a tree of custom functionality that that executes in depth-first order, mainly indended to be executed indefinitely until there are no more useful jobs to run or re-run. For every call to the job's main execution function, the job triggers not only its own custom code, but also that of its children. Jobs run forever until something triggers their termination.

Jobs contain custom code that triggers during certain points in its life-time; During initialization, before child updates, after child updates, on notifications, and when falling out of scope. This allows the user to build complex, interactive systems.

At its heart, this type of functionality is what you see in operating systems with how they handle multiple applications, where each job is analogous to an application, or game engines, where each job is analogous to a game object.

## Design
`jobs` is intended to be minimal. It does not depend on STL, nor any other external library. It contains only the minimum amount of functionality to provide useful data structures. It exposes only the functionality and data structures needed to attain its goal, and keeps the implementation details private.

## Usage
`jobs` has one main class which the user will be interfacing most with, `cc0::job`. Some ease-of-use functionality is provided to get going quickly, such as `cc0::job::run` which implements enough boiler plate code to just be able to make a single call to get the job tree executing. 

## Building
No special adjustments need to be made to build `jobs` except enabling C++11 compatibility or above. Simply include the relevant headers in your code and make sure the headers and source files are available in your compiler search paths. Using `g++` as an example, building is no harder than:

```
g++ -std=c++11 code.cpp jobs/jobs.cpp
```

...where `code.cpp` is an example source file containing the user-defined code, such as program entry point.

## Examples
### Creating and registering custom jobs
In and by themselves, jobs do not perform much meaningful work in relation to the user. Because of this, it is necessary to use inheritance in C++ and overload virtual functions inside the jobs in order for them to perform useful tasks. Using the provided macros `CC0_JOBS_NEW` for deriving from the base `cc0::job` class or `CC0_JOBS_DERIVE` to derive from a derivative of the base `cc0::job` class ensures that the in-house RTTI works as well as enabling instantiation via class name string by registering the class name with a global factory pattern class.
```
#include <iostream>
#include "jobs/jobs.h"

CC0_JOBS_NEW(custom_job)
{
protected:
	void on_tick(uint64_t duration_ns) { // Called every time the parent's `tick` function is called, before the job's childrens' `tick` is called.
		std::cout << '\\';
	}
	void on_tock(uint64_t duration_ns) { // Called every time the parent's `tick` function is called, after the job's childrens' `tick` is called.
		std::cout << '/';
	}
	void on_birth( void ) { // Called only once when this job is added to the job tree.
		std::cout << '+';
	}
	void on_death( void ) { // Called only once when this job is terminated.
		std::cout << '-';
	}
};
```
In order for jobs to run, the user must later instantiate jobs and add them to a job tree. See below examples.

### Adding children
Jobs are actually arranged as trees with one root job. Child jobs can be used to create complex behaviors with parent nodes without introducing hard dependencies.
```
#include <iostream>
#include "jobs/jobs.h"

CC0_JOBS_NEW(custom_child)
{
public:
	int attribute;

protected:
	void on_birth( void ) {
		attribute = 0;
		std::cout << "Child welcomed!" << std::endl;
	}
};

CC0_JOBS_NEW(custom_parent)
{
protected:
	void on_birth( void ) {
		add_child<custom_child>();
	}

	void on_tick( void ) {

	}
};
```

Try not to rely on what order children are arranged in. Only know that they execute after their parent's `on_tick` function, but before their parent's `on_tock` function.

### Running a basic custom job
The `cc0::job::run` function provides the user with an easy-to-use function containing boilerplate code for setting up a root job which does nothing but ensures that there is some child among its children that is still enabled (i.e. not disabled and not terminated). If there is no such child, the job terminates itself and the `cc0::job::run` function is exited.

```
#include <iostream>
#include "jobs/jobs.h"

CC0_JOBS_NEW(class printer)
{
protected:
	void on_birth( void ) {
		std::cout << "Hello, World!" << std::endl;
		kill();
	}
	void on_death( void ) {
		std::cout << "Good bye, cruel world!" << std::endl;
	}
};

int main()
{
	printer root;
	root.run();
	std::cout << "Program terminated" << std::endl;
	return 0;
}
```
The `cc0::job::run` function comes in two variants; One taking a job class as template parameter, and one taking a string as a traditional parameter. The parameter is used to determine what initial job should be created. This job should be some kind of initialization job which adds further children to itself or the root job in order to form a useful application.

### Accessing the job tree
The job tree can be accessed in a variety of different ways. Assume we have access to a job, `j`, at an undefined location in the tree. Other jobs in the tree can be accessed via the following traversal techniques.

Traversing up the tree until the root is hit:
```
const cc0::job *p = j;
while (p->get_parent() != nullptr) {
	p = p->get_parent();
}
// At this location `p` will point to the root node.
```

The root node can be accessed directly without manual traversal:
```
const cc0::job *r = j->get_root();
```

A job's children can be accessed in the following manner:
```
const cc0::job *c = j->get_child();
// `c` will now point to the first of `j`'s children, or null if `j` does not have children.
```

Traversing over a job's children is done in the following manner:
```
const cc0::job *c = j->get_child();
while (c != nullptr) {
	c = c->get_sibling();
}
```

Note that `get_child` only returns the first child in the job's list of children. If there are additional children, these must be accessed via the first child's `get_sibling` function. Further children are accessed the same way as well.

### Type information at runtime (RTTI)
The library mainly passes jobs around as pointers to the base class `job`. However, using the in-house RTTI the user can cast pointers to their proper types:
```
#inlude "jobs/jobs.h"

CC0_JOBS_NEW(custom_parent)
{
protected:
	void on_tock(uint64_t) {
		kill();
	}
};

CC0_JOBS_NEW(custom_child)
{
protected:
	void on_tick(uint64_t) {
		custom_parent *parent = get_parent()->cast<custom_parent>();
	}
};

int main()
{
	custom_parent root;
	root.add_child<custom_child>();
	while (root.is_enabled()) {
		root.tick(0);
	}
	return 0;
}
```
Note that `cast` will return null when a type fails to convert to the requested type.

The same can be done with the safer `ref` class where casting a reference down relies on compile-time knowledge of the inheritance tree and will generate errors when in violation, while casting a reference up can be done via `cast`:
```
#inlude "jobs/jobs.h"

CC0_JOBS_NEW(custom_parent)
{
protected:
	void on_tock(uint64_t) {
		kill();
	}
};

CC0_JOBS_NEW(custom_child)
{
protected:
	void on_tick(uint64_t) {
		cc0::job::ref<custom_parent> parent = get_parent()->get_ref().cast<custom_parent>();
	}
};

int main()
{
	custom_parent root;
	root.add_child<custom_child>();
	while (root.is_enabled()) {
		root.tick(0);
	}
	return 0;
}
```

### Searching for jobs using queries
Jobs often need to select children based off of some criteria. The criteria is essentially custom code wrapped in `do_execute` function inside a filter.
```
#include "jobs/jobs.h"

bool only_enabled(const cc0::job &j)
{
	return j.is_enabled(); // The search criteria, i.e. the job must have existed for 1000 miliseconds.
};

CC0_JOBS_NEW(custom_job)
{
protected:
	void on_birth( void ) {
		for (int i = 0; i < 10; ++i) {
			job *j = add_child<cc0::job>();
			if (i % 1) {
				j->disable();
			}
		}
		cc0::job::query::results r = filter_children(only_enabled); // Returns only enabled children in a result list.
		kill();
	}
};

int main()
{
	custom_job root;
	root.run();
	return 0;
}
```
Queries only apply to a target's children, but technically queries can be applied on any part of the job tree by applying the principles of tree navigation as seen in the examples.

Once a result is returned, the result can be further filtered using `filter_results` on the `results` object. Calling filtering subsequently functions as and boolean logic where results in the final results list must pass all filters to be included.

`get_children` provides the user with a query that is common, i.e. selecting children of a certain type. This is just a convenience function that wraps already existing functionality, but nevertheless proves to make code more readable when performing a common task.
```
CC0_JOBS_NEW(custom_job)
{
protected:
	void on_birth( void ) {
		cc0::job::query::results r = get_children<custom_job>();
	}
};
```
Queries should not use the provided methods for inheritance that would otherwise apply for jobs (i.e. `cc0::inherit`, `CC0_JOBS_NEW`, or `CC0_JOBS_DERIVE`).

There are a few optional ways to apply queries:

One way is to inherit `cc0::job::query` and overloading the public `()` operator (`const`) taking a `const cc0::job&` as parameter and returning a `bool`.
```
class custom_query : public cc0::job::query
{
public:
	bool operator()(const cc0::job &j) const {
		return j.is_enabled();
	}
};
```
Filtering is then done via:
```
CC0_JOBS_NEW(custom_job)
{
protected:
	void on_birth( void ) {
		custom_query q;
		cc0::job::query::results r = filter_children(q).filter_results(q);
	}
};
```

Templates can also be used to apply filters in a more looser manner that does not require the user to inherit a base class:
```
class custom_query_functor
{
public:
	bool operator()(const cc0::job &j) const {
		return j.is_enabled();
	}
};

bool custom_query_function(const cc0::job &j)
{
	return j.is_enabled();
}

CC0_JOBS_NEW(custom_job)
{
protected:
	void on_birth( void ) {
		custom_query_functor ftor;
		cc0::job::query::results r = filter_children(ftor).filter_results(custom_query_function);
	}
};
```

Custom queries can be invoked in two ways when using templates; One way by explicitly instantiating the query object is passing it as a reference, which is useful when objects require some attributes to be set before executing the query, and specifying its type as a template parameter and implicitly instantiating it. The example above shows explicit instantiation, while the one below shows implicit:
```
class custom_query_functor
{
public:
	bool operator()(const cc0::job &j) const {
		return j.is_enabled();
	}
};

CC0_JOBS_NEW(custom_job)
{
protected:
	void on_birth( void ) {
		cc0::job::query::results r = filter_children<custom_query_functor>().filter_results<custom_query_functor>();
	}
};
```

### Referencing an existing job
Any job can access any other job in the tree. Any job may also expire at any time independent of other jobs. This means that there is a need to reference jobs inside other jobs in a safe manner. `jobs` provides a way to reference jobs via `get_ref` in a way to reflect if not only their memory has been freed, and thus, their reference becoming invalidated.

```
#include <iostream>
#include "jobs/jobs.h"

CC0_JOBS_NEW(custom_job)
{
private:
	cc0::job::ref m_child; // A persistent reference.

protected:
	void on_birth( void ) {
		m_child = add_child<cc0::job>()->get_ref(); // The persistent reference is set.
	}
	void on_tick(uint64_t) {
		if (m_child.get_job() != nullptr) {
			std::cout << "Reference is valid" << std::endl;
			m_child.get_job()->kill(); // Once this is called, the reference will automatically become invalid.
		} else {
			std::cout << "Reference is invalid" << std::endl;
			kill();
		}
	}
};
```

As long as the job tree is executing on a single thread a reference to another job can be said to be valid for the duration of the current function call. If the programmer extends the job tree execution to be across multiple treads. In out-of-the-box functionality, references are mainly useful when a job references another job in a persistent manner, i.e. across multiple executions of the job tree.

### Dealing with durations and time
`jobs` currently has poor support for handling of time. However, for interactive systems, the user should be aware of the `duration_ns` parameter passed to both `on_tick` and `on_tock` and scale scaleble work appropriately.

```
#include "jobs/jobs.h"

CC0_JOBS_NEW(timescaled_job)
{
private:
	struct V3 { double x, y, x; };

	V3 m_pos;
	V3 m_ups; // Units per second
	
protected:
	void on_birth( void ) {
		m_pos.x =  0.00;
		m_pos.y =  0.00;
		m_pos.z =  0.00;
		m_dir.x =  0.50;
		m_dir.y =  0.25;
		m_dir.z = -0.50;
	}
	void on_tick(uint64_t duration_ns) {
		const double delta_time = duration_ns / 1000000000.0;
		m_pos.x += m_ups.x * delta_time;
		m_pos.y += m_ups.y * delta_time;
		m_pos.z += m_ups.z * delta_time;
	}
};
```

Using game engines as an example, durations should for instance be used to scale movement over the duration. If the frame rate is high, the movement at each tick should be shorter, and vice versa.

Local time scaling (i.e. time dilation) is not properly supported at the moment. The idea is for each job to keep its own time and the programmer would be able to slow down or speed up its execution (including the execution of its children) independent from the rest of the parent tree and letting the time drift apart from the parent tree.

### Job state terminology
A job can be killed, i.e., marked for deletion (killed jobs are not immediately deleted as this could prove unsafe for other jobs that are still referencing the killed job). This will cease the job's main functionality such as ticking and event handling. A job that has been killed will be registered as such via the `is_killed` flag. Conversely, a job will register as alive via `is_alive` if it has not been killed. A job is killed via the `kill` function.

A job may be enabled or disabled. When disabled, functions such as ticking and event handling are disabled. A job counts as enabled if its enabled flag is set to true. This state of the job can be checked via `is_enabled` and `is_disabled`. A job may be enabled via `enable` and disabled via `disable` functions. When a job has been killed, it will automatically count as disabled as well.

Jobs may sleep for some given amount of time. Sleeping also prevents jobs' main functions from triggering, such as ticking and event handling. This state can be checked via `is_sleeping` and `is_awake`, and can be modified using `sleep` and `wake` respectively. Sleeping jobs will not register the job as disabled.

Since there is overlap between a job being enabled/disabled, awake/asleep, and alive/killed a catch-all concept of 'active' is introduced. A job is active if it is alive, enabled, and awake. This state of the job can be checked via `is_active` and `is_inactive`.

A job may wait, as in skip a tick/tock despite otherwise being active due to settings with its minimum and maximum allowed duration intervals. Whenever a job skips a tick/tock cycle it is known as "waiting" which can be checked via the `is_waiting` flag. Whenever a job does not skip a tick/tock cycle it is known as "ready" which can be checked via the `is_ready` flag. This is a separate concept from active however, meaning that a waiting job does not affect its active state.

### Event handling
Each job has the capability to deal with events which are thrown from some other job in the job tree. Normally events come from either the parent job, or a child job, but could come from elsewhere.

Events are defined as an identifying string passed to the target job together with a reference to the job emitting the event. Events trigger a registered callback in the target job. If no callback is registered (as is the default) nothing will happen. In order to subscribe (i.e. register a callback) to an event `listen` should be used.
```
#include "jobs/jobs.h"

CC0_JOBS_NEW(listener)
{
private:
	uint64_t m_custom_events_received;

private:
	void event_callback(cc0::job &sender) {
		++m_custom_events_received;
	}
protected:
	void on_birth( void ) {
		listen<listener>("custom_event", &listener::event_callback);
	}

public:
	listener( void ) : m_custom_events_received(0) {}
};

CC0_JOBS_NEW(sender)
{
protected:
	void on_tick(uint64_t) {
		notify_parent("custom_event");
	}
};
```
In the above example, instantiating a `sender` as a child under a `listener` will trigger `listener`'s `event_callback` function every time the `sender` object ticks.

Notifications can be sent via `notify_parent` to the parent job, `notify_children` to all child jobs, or `notify_group` to all results from a query. However, using standard tree navigation the user can send event notifications to any job in the tree.

Currently, only one callback can be registered per event. Note that only active jobs can react to events.

Jobs can unsubscribe from events that were previously subscribed to using `ignore`:
```
#include "jobs/jobs.h"

CC0_JOBS_NEW(listener)
{
private:
	uint64_t m_custom_events_received;

private:
	void event_callback(cc0::job &sender) {
		++m_custom_events_received;
		ignore("custom_event")
	}
protected:
	void on_birth( void ) {
		listen<listener>("custom_event", &listener::event_callback);
	}

public:
	listener( void ) : m_custom_events_received(0) {}
};

CC0_JOBS_NEW(sender)
{
protected:
	void on_tick(uint64_t) {
		notify_parent("custom_event");
	}
};
```
In the above example, instantiating a `sender` as a child under a `listener` will trigger `listener`'s `event_callback` function only once when the `sender` object ticks as the callback also unsubscribes from the event via `ignore`.

### Tick frequency/duration limits
By default, the job tree will execute whenever it is called to trigger. However, there are many situations where the user may want to rate limit the ticking frequency. There are two ways to limit the rate at which jobs tick; Using rate limiting, or interval limiting. Both mean the same thing, but are reciprocal, i.e. one version works with Hertz and the other works with the duration of a cycle.

Rate limiting:
```
cc0::job j;

// Limit tick rate to an interval between 20Hz and 80Hz.
j.limit_tick_rate(20, 80);
```

Unlimiting the rate:
```
j.unlimit_tick_rate();
```

Interval limiting:
```
cc0::job j;

// Limit tick duration to an interval between 16 nanoseconds and 32 nanoseconds.
j.limit_tick_duration(16, 32);
```

Unlimiting the interval:
```
j.unlimit_tick_duration();
```

Limited tick rates and intervals function by letting a job sleep or clip time before feeding it into the job. Whenever a shorter duration than the minimally allowed duration is passed to a job's `tick` function, the job waits until enough time has passed to perform the tick. Whenever a duration longer than the maximally allowed duration is passed to a job's `tick` function the duration is clipped to the maximally allowed duration. As an example, waiting ensures that games run no faster than a certain given frame rate, while time clipping will appear to slow the game down in order to avoid that extremely low frame rates move the game too far along between each frame so as to become unplayable. These qualities may be essential to, for instance, real-time physics simulations which can break down at very small or very large durations.

Duration data for the root node is provided by the user, who can then decide whether to fix the elapsed time between ticks to some given value, or base the value on real time elapsed between ticks. 

Note that waiting due to durations lower than the accepted minimum is implemented as a soft sleep, meaning that the thread executing the job tree will not stop processing job nodes, but will skip waiting ones. Jobs waiting due to durations will not be marked as sleeping. However, the convenience function `cc0::job::run` is an exception in this regard as it does a hard sleep, i.e. sleeps the executing thread in order to allow the thread to context switch and lower power consumption. The root node will still not be marked as sleeping in this case.

### Running the job tree until complete
Complex jobs may not terminate after a deterministic amount of time. In order to run such jobs to completion the user must run the root of the job tree until some condition has been fulfilled. The example below shows how such a root can be set up, and what convenience functions are provided.

First, the user must determine what the conditions are for the job to finish. In this example, when all child jobs have been terminated, the parent node will also be terminated, making the completion of the job. Such a parent/root node looks like the following:
```
CC0_JOBS_NEW(fork)
{
protected:
	void on_tick(uint64_t) {
		if (has_enabled_children()) {
			kill();
		}
	}
};
```

Second, some sub-jobs must be set up. This is only a trivial jobs for the purposes of illustration.
```
CC0_JOBS_NEW(counter)
{
private:
	uint64_t m_countdown;

protected:
	void on_tick(uint64_t) {
		if (m_countdown == 0) {
			kill();
		} else {
			--m_countdown;
		}
	}

public:
	counter( void ) : m_countdown(0) {}

	void set_countdown(uint64_t countdown) {
		m_countdown = countdown;
	}
};
```

Next, the jobs need to be attached to the root job as children:
```
fork root;
for (uint64_t i = 0; i < 100; ++i) {
	root.add_child<counter>()->set_countdown(i+1);
}
```

Finally, the tree must execute:
```
fork.run();
```

`cc0::job::run` executes the tree until the provided root node (input parameter) is marked as disabled. In the example above, each child job will decrement a counter which, when hitting 0, will terminate the child job thereby marking it as disabled. The root node checks if there are any enabled children at each tick. When it does not detect a single enabled child, it terminates itself thereby marking it as disabled and returning from `cc0::job::run`.

## Limitations
`jobs` is not trivially threadable in an effective manner since any job may read or write to any other job.

`jobs` does not group jobs of the same type together to aid the compiler emitting SIMD instructions, since jobs are executed in depth-first order rather than by type.

Due to how different C++ compilers work, it may be necessary to use a job class in some way before it will be automatically registered with the job factory (the data structure responsible for enabling job class instantiation via identifier string) since C++ does not guarantee that global static variables are initialized before `main`. This issue may present itself as the failure to instantiate a class via its identifier string (returns null on allocation) even though the job class has been registered in-code since the compiler has deferred running that code to some point after the attempted instantiation.

## TODO
* Registering multiple callbacks per event.
* Removing only one subscribed event callback rather than all for a single event.
* Functional time scaling.
* Instead of capping durations if they exceed the maximum allowed duration, maybe we should temporarily scale the job's time scale down to match the target time cap, although this would also affect children.
* Time scaling should be local to the object making the call for a job to sleep. Calling `j.sleep(100)` should use `this`'s definition of `100` rather than `j`'s.
