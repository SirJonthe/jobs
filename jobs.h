/// @file
/// @author github.com/SirJonthe
/// @date 2022, 2023
/// @copyright Public domain.
/// @license CC0 1.0

#ifndef CC0_JOBS_H_INCLUDED__
#define CC0_JOBS_H_INCLUDED__

#include <cstdint>

#define CC0_JOBS_REGISTER(job_name) static const bool job_name##_registered = cc0::jobs::job::register_job<job_name>(#job_name);

#define CC0_JOBS_INSTANCE(job_name_str) cc0::jobs::job::instance_job(job_name_str);

namespace cc0
{
	namespace jobs
	{
		/// @brief Generates a new UUID.
		/// @return A new UUID.
		uint64_t new_uuid( void );

		/// @brief Definitions used within the library itself.
		/// @note For internal use only. Everything inside this namespace could be subject to change without notice.
		namespace internal
		{
			/// @brief The base class for basic RTTI within the package.
			class rtti
			{	
			protected:
				/// @brief Returns the self referencing pointer if the provided type ID matches the class ID.
				/// @param type_id The provided type ID.
				/// @return The self referencing pointer. Null if type ID does not match class ID.
				virtual void *self(uint64_t type_id);

				/// @brief Returns the self referencing pointer if the provided type ID matches the class ID.
				/// @param type_id The provided type ID.
				/// @return The self referencing pointer. Null if type ID does not match class ID.
				virtual const void *self(uint64_t type_id) const;

			public:
				/// @brief Returns a unique ID for this specific class.
				/// @return A unique ID for this specific class.
				static uint64_t type_id( void );

				/// @brief Returns a pointer to a type in the inheritance chain (forwards and backwards).
				/// @tparam type_t The requested type.
				/// @return A pointer to a type in the inheritance chain (forwards and backwards). Null if the requested type is not in the inheritance chain.
				template < typename type_t >
				type_t *cast( void );

				/// @brief Returns a pointer to a type in the inheritance chain (forwards and backwards).
				/// @tparam type_t The requested type.
				/// @return A pointer to a type in the inheritance chain (forwards and backwards). Null if the requested type is not in the inheritance chain.
				template < typename type_t >
				const type_t *cast( void ) const;

				/// @brief Creates a new instance of the RTTI class.
				/// @return The new instance.
				static rtti *instance( void );
			};
		}

		/// @brief A helper class that will ensure a working in-house RTTI when inheriting from job classes.
		/// @tparam self_t The derived class.
		/// @tparam base_t The class derived from (must itself be a derivative of jobs::inherit).
		/// @note For RTTI to work, the user must indirectly inherit jobs and tasks via the inherit class.
		template < typename self_t, typename base_t >
		class inherit : public base_t
		{	
		protected:
			/// @brief Returns the self referencing pointer if the provided type ID matches the class ID.
			/// @param type_id The provided type ID.
			/// @return The self referencing pointer. Null if type ID does not match class ID.
			void *self(uint64_t type_id);

			/// @brief Returns the self referencing pointer if the provided type ID matches the class ID.
			/// @param type_id The provided type ID.
			/// @return The self referencing pointer. Null if type ID does not match class ID.
			const void *self(uint64_t type_id) const;
		
		public:
			/// @brief Returns a unique ID for this specific class.
			/// @return A unique ID for this specific class.
			static uint64_t type_id( void );

			/// @brief Creates a new instance of the the self_t class.
			/// @return The new instance.
			static internal::rtti *instance( void );
		};

		/// @brief An ease-of-use typedef for the function pointer signature used to instantiate job class derivatives.
		typedef cc0::jobs::internal::rtti* (*instance_fn)(void);

		/// @brief A job. Updates itself and its children using custom code that can be inserted via overloading virtual functions within the class.
		class job : public inherit<job, internal::rtti>
		{
		private:
			/// @brief Shared data used by automatic reference counting.
			struct shared
			{
				uint64_t watchers;
				bool     deleted;
			};

			/// @brief A class capable of instantiating jobs based on the name of the class string.
			class factory
			{
			private:
				/// @brief A binary search tree mapping names of job class derivatives to functions instantiating them.
				class inventory
				{
				private:
					struct node
					{
						uint64_t     key;
						const char  *name;
						instance_fn  instance;
						node        *lte;
						node        *gt;
					};

				private:
					node *m_root;
				
				private:
					/// @brief Creates a key from the string.
					/// @param s The string.
					/// @return The key.
					uint64_t make_key(const char *s) const;

					/// @brief Compares two strings.
					/// @param a A string.
					/// @param b Another string.
					/// @return True if the strings are exactly equal.
					bool str_cmp(const char *a, const char *b) const;

					/// @brief Deletes memory of specified node and sub-nodes.
					/// @param n The node to delete.
					void free_node(node *n);

				public:
					/// @brief Initializes inventory.
					inventory( void );

					/// @brief Frees memory in inventory.
					~inventory( void );

					/// @brief Adds a new 
					/// @param name The name of the job class derivative that will be used to call the instance function.
					/// @param instance The instance function that allocates memory for the specified job class derivative.
					/// @note If there is a key conflict a new node is added. The old one is not overwritten.
					void add_job(const char *name, instance_fn instance);

					/// @brief Allocates memory for the job class derivative with the specified name.
					/// @param name The name of the job class derivative to allocate memory for.
					/// @return Returns null when the specified job class derivative is not stored in the inventory.
					cc0::jobs::internal::rtti *instance_job(const char *name);
				};

			private:
				static inventory m_products;

			public:
				/// @brief Adds the job class derivative to the factory in order for it to be able to be instantiated later.
				/// @tparam job_t The type of the job class derivative.
				/// @param name The name of the job class derivative as a string that can later be used to call the instantiation function.
				template < typename job_t >
				static void register_job(const char *name);

				/// @brief Calls the appropriate instantiation function based on the provided job class derivative name.
				/// @param name The name of the job class derivative to instantiate.
				/// @return An allocated job class derivative based on the provided name. Null if there is no instantiation function for the provided name.
				static job *instance_job(const char *name);
			};

		public:
			/// @brief Safely references a job. Will yield null if the referenced job has been destroyed.
			class ref
			{
				friend class job;

			private:
				job    *m_job;
				shared *m_shared;
			
			public:
				/// @brief Initializes the reference.
				/// @param p The job to reference.
				explicit ref(job *p = nullptr);

				/// @brief Copy a reference.
				/// @param r The reference to copy.
				ref(const ref &r);

				/// @brief Move a reference.
				/// @param r The reference to move.
				ref(ref &&r);
				
				/// @brief Destroys the reference.
				~ref( void );

				/// @brief Copy a reference.
				/// @param r The reference to copy.
				/// @return The modified object.
				ref &operator=(const ref &r);

				/// @brief Move a reference.
				/// @param r The reference to move.
				/// @return The modified object.
				ref &operator=(ref &&r);

				/// @brief References a new job.
				/// @param p The job to reference.
				void set_ref(job *p);

				/// @brief Releases the reference. Deletes the metadata memory if this is the last reference referencing it. 
				void release( void );

				/// @brief Returns the job.
				/// @return The job. Null if the job has been deleted.
				job *get_job( void );

				/// @brief Returns the job.
				/// @return The job. Null if the job has been deleted.
				const job *get_job( void ) const;
			};

			/// @brief A search query containing a number of filters executed in sequence on the subject's children.
			/// @note Filters are alternative, meaning if a job fits any of the filters, then the job is selected.
			class query
			{
			public:
				/// @brief A query filter. Add more of these to the query to specify your search. Do this by creating a new class and inheriting this filter class, then overloading the () operatior.
				class filter : public inherit<filter, internal::rtti>
				{
				private:
					filter *m_or;
					filter *m_and;
				
				protected:
					// TODO DOC
					/// @brief 
					/// @param j 
					/// @return 
					virtual bool do_execute(const job &j) const;

				public:
					// TODO DOC
					/// @brief 
					filter( void );

					// TODO DOC
					/// @brief 
					virtual ~filter( void );

					/// @brief The filter function.
					/// @param j The job to determine if it should be included in the search results.
					/// @return True if the job should be included in the search results.
					bool operator()(const job &j) const;

					/// @brief Sets the AND filter.
					/// @tparam filter_t The filter class to use.
					/// @return The newly created filter.
					template < typename filter_t >
					filter_t &set_and_filter( void );

					/// @brief Sets the OR filter.
					/// @tparam filter_t The filter class to use.
					/// @return The newly created filter.
					template < typename filter_t >
					filter_t &set_or_filter( void );
				};

				/// @brief A single result from a search query.
				class result
				{
				private:
					job::ref   m_job;
					result    *m_next;
					result   **m_prev;
				
				public:
					/// @brief Constructs a result.
					/// @param j The job to store in the result.
					result(job &j, result **prev);

					/// @brief Destroys the next result in the chain.
					~result( void );

					/// @brief Copy constructor.
					/// @param NA The result to copy. 
					result(const result&) = default;

					/// @brief Assignment operator.
					/// @return The result to copy.
					result &operator=(const result&) = default;
					
					/// @brief Returns the job stored in the result.
					/// @return The job stored in the result.
					job::ref get_job( void );

					/// @brief Returns the next result in the chain.
					/// @return The next result in the chain.
					result *get_next( void );

					/// @brief Returns the previous result in the chain.
					/// @return The previous result in the chain.
					result *get_prev( void );

					/// @brief Deletes memory and removes result from chain of results.
					/// @return The next result in the chain.
					result *remove( void );
				};

				/// @brief All results from a search query.
				class results
				{
				public:
					result *m_first;
				
				public:
					/// @brief Default constructor.
					results( void );

					/// @brief Deletes memory.
					~results( void );

					/// @brief Returns the first result in the collection of results.
					/// @return The first result in the collection of results.
					result *get_results( void );
				};

			private:
				job::ref  m_subject;
				filter   *m_filter;

			public:
				/// @brief Constructs an empty query with the specified job as the subject (database).
				/// @param subject The subject to use as a database when searching for children matching the filters.
				query(job *subject);

				/// @brief Frees data in query.
				~query( void );

				/// @brief Sets a filter to the filter chain.
				/// @tparam filter_t The filter class to use.
				template < typename filter_t >
				filter_t &set_filter( void );
				
				/// @brief Applies the filters to the children of the subject and selects only those which match the filters.
				/// @return The search results after performing the query.
				results execute( void );
			};

		private:
			job      *m_parent;
			job      *m_sibling;
			job      *m_child;
			uint64_t  m_pid;
			uint64_t  m_sleep;
			uint64_t  m_existed_for;
			uint64_t  m_active_for;
			uint64_t  m_existed_tick_count;
			uint64_t  m_active_tick_count;
			uint64_t  m_time_scale;
			shared   *m_shared;
			bool      m_enabled;
			bool      m_kill;
		
		private:
			/// @brief Generates a new, unique job ID. Just increments a counter.
			/// @return The new job ID.
			static uint64_t new_pid( void );

			/// @brief  Tells the shared object that the referenced object has been deleted.
			void set_deleted( void );

			/// @brief Adds a job as a sibling. If specified sibling location is null, then the job is added there, otherwise it recursively traverses to the next sibling location.
			/// @param loc The sibling location.
			/// @param p The job to add as a sibling.
			void add_sibling(job *&loc, job *p);

			/// @brief Deletes all siblings.
			/// @param siblings The first sibling in the list of siblings.
			void delete_siblings(job *&siblings);

			/// @brief Deletes all children and grandchildren.
			/// @param children The first child in the list of children.
			void delete_children(job *&children);

			/// @brief Deletes all children that have been marked as killed.
			/// @param child The current child in the list.
			void delete_killed_children(job *&child);

			/// @brief Ticks children.
			/// @param duration The time elapsed.
			void tick_children(uint64_t duration);

			/// @brief Scales a time.
			/// @param time The time to scale.
			/// @param time_scale The fixed-point scale, shifted by 32 bits.
			/// @return The scaled time.
			static uint64_t scale_time(uint64_t time, uint64_t time_scale);
		/// @brief Called when ticking the job, before the children are ticked.

		protected:
			/// @param duration The time elapsed.
			/// @note There is no default behavior. This must be overloaded.
			virtual void on_tick(uint64_t duration);
			
			/// @brief Called when ticking the job, after the children have been ticked.
			/// @param duration The time elapsed.
			/// @note There is no default behavior. This must be overloaded.
			virtual void on_tock(uint64_t duration);

			/// @brief Called immediately when the job is created.
			/// @note There is no default behavior. This must be overloaded.
			virtual void on_birth( void );

			/// @brief Called immediately when the job is killed.
			/// @note There is no default behavior. This must be overloaded.
			virtual void on_death( void );

			/// @brief The function that jobes an incoming message via one of the notify functions.
			/// @param event The event string.
			/// @param sender The sender.
			/// @note There is no default behavior. This must be overloaded.
			virtual void on_message(const char *event, job *sender);

		public:
			/// @brief Initializes the job.
			job( void );

			/// @brief Destroys the children, then the next sibling.
			~job( void );

			/// @brief Calls on_tick, ticks all children, and on_tock.
			/// @param duration The time elapsed.
			void tick(uint64_t duration);
			
			/// @brief Queues the job for destruction. The 'on_death' function is called immediately (if the object is active), but the memory for the job may not be freed immediately.
			/// @note Kills all children first, then kills the parent job. The 'death' function is only called if the job is active.
			void kill( void );

			/// @brief Kills all children of the node, envoking the 'on_death' function in each.
			void kill_children( void );
			
			/// @brief Lets the job sleep for a given amount of time. If the job is already sleeping only the difference in time is added to the sleep duration (if time is larger than the current sleep duration).
			/// @param time The amount of time to sleep.
			void sleep(uint64_t time);

			/// @brief Disables sleep.
			void wake( void );

			/// @brief Adds an event for the job to listen and respond to.
			// void listen(const char *event_id);

			/// @brief Adds a child to the job' list of children.
			/// @tparam job_t The type of the child to add to the job.
			/// @return The job that was added.
			template < typename job_t >
			job_t &add_child( void );

			/// @brief Enables the job, allowing it to tick and call the death function.
			void enable( void );

			/// @brief Disables the job, disabling ticking and death function.
			void disable( void );

			/// @brief Checks if the job has been killed.
			/// @return True if the job has been killed.
			bool is_killed( void ) const;

			/// @brief Checks if the job is not killed.
			/// @return True if the job is not killed.
			bool is_alive( void ) const;

			/// @brief Checks if the job is enabled.
			/// @return True if the job is enabled.
			/// @note Killing an object automatically sets the job to disabled.
			bool is_enabled( void ) const;

			/// @brief Checks if the job is disabled.
			/// @return True if the job is disabled.
			/// @note Killing an object automatically sets the job to disabled.
			bool is_disabled( void ) const;
			
			/// @brief Checks if the job is sleeping.
			/// @return True if the job is sleeping.
			bool is_sleeping( void ) const;

			/// @brief Checks if the job is awake.
			/// @return True if the job is awake.
			bool is_awake( void ) const;
			
			/// @brief Checks if the job is currently active.
			/// @return True if the job is neither killed, disabled, or asleep.
			bool is_active( void ) const;

			/// @brief Checks if the job is currently inactive.
			/// @return True if the job is either killed, disabled, or asleep.
			bool is_inactive( void ) const;

			/// @brief Returns the job ID.
			/// @return The job ID.
			uint64_t get_pid( void ) const;

			/// @brief Notifies the parent of an event.
			/// @param event The event string.
			/// @note Nothing will happen if the job is not active.
			void notify_parent(const char *event);

			/// @brief Notify all children of an event.
			/// @param event The event string.
			/// @note Nothing will happen if the job is not active.
			void notify_children(const char *event);

			/// @brief Notify all entries in a group of an event.
			/// @param event The event string.
			/// @param group The group to be notified.
			/// @note Nothing will happen if the job is not active.
			void notify_group(const char *event, job::query::results &group);

			/// @brief Notify the job of an event.
			/// @param event The event string.
			/// @param sender The job sending the event.
			/// @note Nothing will happen if the job is not active. 
			void notify(const char *event, job *sender);

			/// @brief Returns a safe reference that will automatically turn null if the job is deleted.
			/// @return The reference.
			ref get_ref( void );

			/// @brief Gets the accumulated time the job has existed for.
			/// @return The accumulated time the job has existed for.
			uint64_t get_existed_for( void ) const;

			/// @brief Gets the accumulated time the job has been active for.
			/// @return The accumulated time the job has been active for.
			uint64_t get_active_for( void ) const;

			/// @brief Gets the accumulated number of ticks the job has existed for.
			/// @return The accumulated number of ticks the job has existed for.
			uint64_t get_existed_tick_count( void ) const;

			/// @brief Gets the accumulated number of ticks the job has been active for.
			/// @return The accumulated number of ticks the job has been active for.
			uint64_t get_active_tick_count( void ) const;

			/// @brief Returns the parent job.
			/// @return The parent job.
			job *get_parent( void );

			/// @brief Returns the parent job.
			/// @return The parent job.
			const job *get_parent( void ) const;

			/// @brief Returns the first child job.
			/// @return The first child job.
			job *get_child( void );

			/// @brief Returns the first child job.
			/// @return The first child job.
			const job *get_child( void ) const;

			/// @brief Returns the next sibling job.
			/// @return The next sibling job.
			job *get_sibling( void );

			/// @brief Returns the next sibling job.
			/// @return The next sibling job.
			const job *get_sibling( void ) const;

			/// @brief Returns the root node of the entire job tree.
			/// @return The root node of the entire job tree.
			job *get_root( void );

			/// @brief Returns the root node of the entire job tree.
			/// @return The root node of the entire job tree.
			const job *get_root( void ) const;

			/// @brief Sets the time scale for this job.
			/// @param time_scale The desired time scaling.
			void set_time_scale(float time_scale);

			/// @brief Gets the current time scale for this job.
			/// @return The current time scale for this job.
			float get_time_scale( void ) const;

			/// @brief Returns a default query (no filters) using this job as the subject of the search.
			/// @return A default query without filters.
			query search( void );

			/// @brief Returns a query filtering out children not of the requested job type.
			/// @tparam job_t The children of the job matching job type.
			/// @return A query filtering out children not of the requested job type.
			template < typename job_t >
			query get_children( void );

			/// @brief Adds the job class derivative to the factory in order for it to be able to be instantiated later.
			/// @tparam job_t The type of the job class derivative.
			/// @param name The name of the job class derivative as a string that can later be used to call the instantiation function.
			/// @return Always true.
			/// @note Do not use this function directly. Instead use CC0_JOBS_REGISTER.
			/// @sa CC0_JOBS_REGISTER
			template < typename job_t >
			static bool register_job(const char *name);

			/// @brief Calls the appropriate instantiation function based on the provided job class derivative name.
			/// @param name The name of the job class derivative to instantiate.
			/// @return An allocated job class derivative based on the provided name. Null if there is no instantiation function for the provided name.
			/// @note Do not use this function directly. Instead use CC0_JOBS_INSTANCE.
			/// @sa CC0_JOBS_INSTANCE
			static job *instance_job(const char *name);
		};
		CC0_JOBS_REGISTER(job)

		/// @brief A job node that monitors its children and kills execution when there are no remaining children.
		class jobs : public inherit<jobs, job>
		{
		private:
			uint64_t m_min_duration;
			uint64_t m_max_duration;
			uint64_t m_duration;

		protected:
			/// @brief Starts timer. Terminates execution if it has no children.
			/// @param duration The time elapsed since last tick.
			void on_tick(uint64_t duration);

			// TODO IMPL
			/// @brief Sleeps the sub-tree if the execution time is faster than the minimum ticks per second.
			/// @param duration The time elapsed since last tick.
			//void on_tock(uint64_t duration);
		
		public:
			/// @brief  Default constructor. No tick limits.
			jobs( void );
			
			/// @brief Constructs the tree with tick limits.
			/// @param min_ticks_per_sec The minimum number of ticks that will be performed per second. If the jobes do not hit the target, the durations are clipped to the 1000 / min_ticks_per_sec.
			/// @param max_ticks_per_sec The maximum number of ticks that will be performed per second. If the jobes exceed the target the tree sleeps.
			jobs(uint64_t min_ticks_per_sec, uint64_t max_ticks_per_sec);

			// TODO IMPL
			/// @brief 
			/// @param  
			//void tick( void );
		};
		CC0_JOBS_REGISTER(jobs)

		/// @brief Spawns a root job node and attaches the specified initial job as a child to that node, then continues execution until the root node no longer has any children.
		/// @tparam init_job_t The initial job to attach as a child to the root node.
		/// @note The init job should set up everything for proper further execution of the jobs.
		template < typename init_job_t >
		void run( void );
	}
}

template < typename type_t >
type_t *cc0::jobs::internal::rtti::cast( void )
{
	return reinterpret_cast<type_t*>(self(type_t::type_id()));
}

template < typename type_t >
const type_t *cc0::jobs::internal::rtti::cast( void ) const
{
	return reinterpret_cast<const type_t*>(self(type_t::type_id()));
}

template < typename self_t, typename base_t >
void *cc0::jobs::inherit<self_t, base_t>::self(uint64_t type_id)
{
	return this->type_id() == type_id ? this : base_t::self(type_id);
}

template < typename self_t, typename base_t >
const void *cc0::jobs::inherit<self_t, base_t>::self(uint64_t type_id) const
{
	return this->type_id() == type_id ? this : base_t::self(type_id);
}

template < typename self_t, typename base_t >
cc0::jobs::internal::rtti *cc0::jobs::inherit<self_t,base_t>::instance( void )
{
	return new self_t;
}

template < typename self_t, typename base_t >
uint64_t cc0::jobs::inherit<self_t, base_t>::type_id( void )
{
	static const uint64_t id = cc0::jobs::new_uuid();
	return id;
}

template < typename job_t >
void cc0::jobs::job::factory::register_job(const char *name)
{
	factory::m_products.add_job(name, job_t::instance);
}

template < typename filter_t >
filter_t &cc0::jobs::job::query::filter::set_and_filter( void )
{
	if (m_and != nullptr) {
		delete m_and;
	}
	filter_t *f = new filter_t;
	m_and = f;
	return *f;
}

template < typename filter_t >
filter_t &cc0::jobs::job::query::filter::set_or_filter( void )
{
	if (m_or != nullptr) {
		delete m_or;
	}
	filter_t *f = new filter_t;
	m_or = f;
	return *f;
}

template < typename filter_t >
filter_t &cc0::jobs::job::query::set_filter( void )
{
	if (m_filter != nullptr) {
		delete m_filter;
	}
	filter_t *f = new filter_t;
	m_filter = f;
	return *f;
}

template < typename job_t >
job_t &cc0::jobs::job::add_child( void )
{
	job_t *p = nullptr;
	if (!is_killed()) {
		p = new job_t;
		add_sibling(m_child, p);
		((job*)p)->on_birth();
	}
	return *p;
}

template < typename job_t >
cc0::jobs::job::query cc0::jobs::job::get_children( void )
{
	class type_filter : public inherit<type_filter, job::query::filter>
	{
	protected:
		bool do_execute(const job &j) const { return j.cast<job_t>() != nullptr; }
	};
	query q;
	q.set_filter<type_filter>();
	return q;
}

template < typename job_t >
bool cc0::jobs::job::register_job(const char *name)
{
	factory::register_job<job_t>(name);
	return true;
}

template < typename init_job_t >
void cc0::jobs::run( void )
{
	cc0::jobs::jobs j;
	j.add_child<init_job_t>();
	while (j.is_enabled()) {
		j.tick(0);
	}
}

#endif
