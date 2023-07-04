# jobs
## Copyright
Public domain, 2023

github.com/SirJonthe

## About
`jobs` is a minimalist library that introduces a the concept of jobs, or tasks, to C++11.

A job, or a task, within the context of this library, is a tree of custom functionality that that executes in depth-first order. For every call to the job's main execution function, the job triggers not only its own custom code, but also that of its children. Jobs run forever until something triggers their termination.

Jobs contain custom code that triggers during certain points in its life-time; During initialization, before child updates, after child updates, on notifications, and when falling out of scope. This allows the user to build complex, interactive systems.

At its heart, this type of functionality is what you see in operating systems with how they handle multiple applications, where each job is analogous to an application, or a game engine, where each job is analogous to a game object.

## Design
`jobs` is intended to be minimal. It does not depend on STL, nor any other external library. It contains only the minimum amount of functionality to provide useful data structures. It exposes only the functionality and data structures needed to attain its goal, and keeps the implementation details private.

## Usage
Some ease-of-use functionality is provided to get going quickly.

## Building
No special adjustments need to be made to build `jobs` except enabling C++11 compatibility or above. Simply include the relevant headers in your code and make sure the headers and source files are available in your compiler search paths. Using `g++` as an example, building is no harder than:

```
g++ -std=c++11 code.cpp jobs/jobs.cpp
```

...where `code.cpp` is an example source file containing the user-defined code, such as program entry point.

## Examples
### Creating and registering custom jobs
In and by themselves, jobs do not perform much meaningful work in relation to the user. Because of this, it is necessary to use inheritance in C++ and overload virtual functions inside the jobs in order for them to perform useful tasks. The user may also "register" the name of such a custom job so that the user later than instantiate that same job using only its name as a string.
```
#include <iostream>
#include "jobs/jobs.h"

class custom_job : public cc0::jobs::inherit<custom_job, cc0::jobs::job>
{
protected:
	void on_tick(uint64_t duration) { // Called every time the parent's `tick` function is called, before the job's childrens' `tick` is called.
		std::cout << '\\';
	}
	void on_tock(uint64_t duration) { // Called every time the parent's `tick` function is called, after the job's childrens' `tick` is called.
		std::cout << '/';
	}
	void on_birth( void ) { // Called only once when this job is added to the job tree.
		std::cout << '+';
	}
	void on_death( void ) { // Called only once when this job is terminated.
		std::cout << '-';
	}
};
CC0_JOBS_REGISTER(custom_job)
```
Notice that the user must not directly inherit from the `job` base class, or any dirivative thereof, but instead do indirect inheritance via the `inherit` template class as with the example above. This ensures that in-house RTTI works.

In order for jobs to run, the user must later instantiate jobs and add them to a job tree. See below examples.

### Adding children
Jobs are actually arranged as trees with one root job. Child jobs can be used to create complex behaviors with parent nodes without introducing hard dependencies.
```
#include <iostream>
#include "jobs/jobs.h"

class custom_child : public cc0::jobs::inherit<custom_child, cc0::jobs::job>
{
public:
	int attribute;

protected:
	void on_birth( void ) {
		attribute = 0;
		std::cout << "Child welcomed!" << std::endl;
	}
};
CC0_JOBS_REGISTER(custom_child)

class custom_parent : public cc0::jobs::inherit<custom_parent, cc0::jobs::job>
{
protected:
	void on_birth( void ) {
		add_child<custom_child>();
	}

	void on_tick( void ) {

	}
};
CC0_JOBS_REGISTER(custom_parent)
```

Try not to rely on what order children are arranged in. Only know that they execute after their parent's `on_tick` function, but before their parent's `on_tock` function.

### Running a basic custom job
The `run` function provides the user with an easy-to-use function containing boilerplate code for setting up a root job which does nothing but ensures that there is some child among its children that is still enabled (i.e. not disabled and not terminated). If there is no such child, the job terminates itself and the `run` function is exited.

```
#include <iostream>
#include "jobs/jobs.h"

class printer : public cc0::jobs::inherit<printer, cc0::jobs::job>
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
CC0_JOBS_REGISTER(printer)

int main()
{
	cc0::jobs::run<printer>();
	std::cout << "Program terminated" << std::endl;
	return 0;
}
```
The `run` function comes in two variants; One taking a job class as template parameter, and one taking a string as a traditional parameter. The parameter is used to determine what initial job should be created. This job should be some kind of initialization job which adds further children to itself or the root job in order to form a useful application.

### Accessing the job tree
The job tree can be accessed in a variety of different ways. Assume

get_root
get_parent
get_child
get_sibling

Note that `get_child` only returns the first child in the job's list of children. If there are additional children, these must be accessed via the first child's `get_sibling` function. Further children are accessed the same way as well.

### Type information at runtime (RTTI)
The library mainly passes jobs around as pointers to the base class `job`. However, using the in-house RTTI the user can cast pointers to their proper types:
```
#include <iostream>
#inlude "jobs/jobs.h"

class custom_job : public cc0::jobs::inherit<custom_job, cc0::jobs::job>
{};
CC0_JOBS_REGISTER(custom_job)
```

### Searching for jobs using queries
Jobs often need to select children based off of some criteria. The criteria is essentially custom code wrapped in `do_execute` function inside a filter.
```
#include "jobs/jobs.h"

class custom_filter : public cc0::jobs::query::filter
{
public:
	bool do_execute(const job &j) const {
		return j.get_existed_for() > 10000; // The search criteria, i.e. the job must have existed for 1000 miliseconds.
	}
};

class custom_job : public cc0::jobs::inherit<custom_job, cc0::jobs::job>
{
protected:
	void on_birth( void ) {
		for (int i = 0; i < 10; ++i) {
			add_child<cc0::jobs::job>();
		}
	}
	void on_tick(uint64_t duration) {
		
		if (get_exited_tick_count() % 10000 == 0) {
			add_child<cc0::jobs::job>();
		}
		
		query q = search();
		q.add_term<custom_filter>();
		query::results res = q.execute();
		if (res.count_results() >= 10) {
			kill();
		}
	}
};
CC0_JOBS_REGISTER(custom_job)

int main()
{
	cc0::jobs::run<custom_job>();
	return 0;
}
```
Queries apply to a target's children, but technically queries can be applied

`get_children` provides the user with a query that is common, i.e. selecting children of a certain type. This is just a conveinience function that wraps already existing functionality, but nevertheless proves to make code more readable when performing a common task.
```


```
Filters should not use the provided methods for inheritance that would otherwise apply for jobs. The same goes for registering, which should not be done for filters.

### Referencing an existing job
Any job can access any other job in the tree. Any job may also expire at any time independent of other jobs. This means that there is a need to reference jobs inside other jobs in a safe manner.

### Dealing with durations and time
`jobs` currently has poor support for time, but there are still some 

### Dangers
Beware of stale references.

Beware of adding children to a parent when a child dies.

## TODO
* More and better documentation
* Manipulate query results, such as joining two lists together
* A single lambda function instead of queries and filters.
* Too much nested namespaces for the end-user?
* Better way of handling notifications between jobs, such as via a hash table of functionality.
* Better, pre-typecasted references
* Functional time scaling