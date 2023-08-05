/// @file
/// @author github.com/SirJonthe
/// @date 2022, 2023
/// @copyright Public domain.
/// @license CC0 1.0

#ifndef CC0_JOBS_H_INCLUDED__
#define CC0_JOBS_H_INCLUDED__

#include <cstdint>

/// @brief Emits boiler-plate code for creating a new class of job that inherits from another class of job.
/// @param job_name The name of the new class of job.
/// @param base_type The name of the class of job to derive from.
#define CC0_JOBS_DERIVE(job_name, base_type) \
	class job_name; \
	template <> struct cc0::jobs_internal::rtti::type_info<job_name> { static const char *name( void ) { return #job_name; } }; \
	class job_name : public cc0::jobs_internal::inherit<job_name, cc0::jobs_internal::rtti::type_info<job_name>, base_type>

/// @brief Emits boiler-plate code for creating a new class of job that inherits from the default job base class.
/// @param job_name The name of the new class of job.
#define CC0_JOBS_NEW(job_name) \
	CC0_JOBS_DERIVE(job_name, cc0::job)

namespace cc0
{
	class job; // Forward declaration.

	/// @brief Definitions used within the library itself.
	/// @note For internal use only. Everything inside this namespace could be subject to change without notice.
	namespace jobs_internal
	{
		/// @brief Generates a new UUID.
		/// @return A new UUID.
		uint64_t new_uuid( void );

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
			/// @brief Does nothing.
			virtual ~rtti( void );

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

			/// @brief Returns the type ID of the object instance (polymorphic).
			/// @return The type ID of the object instance.
			virtual uint64_t object_id( void ) const;

			/// @brief Returns the type name of the object instance (polymorphic).
			/// @return The type name of the object instance.
			virtual const char *object_name( void ) const;

			/// @brief Returns the type name of this specific class.
			/// @return The type name of this specific class.
			static const char *type_name( void );

			/// @brief Used to make template specializations in order to fetch the name of a type at compile-time.
			/// @tparam type_t The type.
			/// @note This is a work-around for older C++ standards where immediate strings can not be passed as template parameters and the compiler/linker is sensitive to passing global char arrays as template parameters.
			/// @note Every job type inheriting via the inherit class must, in a prior stage, create a template specialization from this using the to-be-declared class, and define a static name() member function returning a const char* string containing the name of the to-be-declared class.
			/// @warning This class should not be used directly by the user. CC0_JOBS_NEW and CC0_JOBS_DERIVE implicitly do all the work.
			/// @sa cc0::jobs_internal::inherit
			/// @sa CC0_JOBS_NEW
			/// @sa CC0_JOBS_DERIVE
			template < typename type_t > struct type_info {};
		};

		/// @brief A binary search tree mapping names of job class derivatives to functions instantiating them.
		template < typename type_t, typename key_t = const char* >
		class search_tree
		{
		private:
			struct node
			{
				uint64_t  hash;
				key_t     key;
				node     *lte;
				node     *gt;
				type_t    value;
			};

		private:
			node *m_root;
		
		private:
			/// @brief Creates a has from the string.
			/// @tparam k_t The key type.
			/// @param s The string.
			/// @return The hash.
			template < typename k_t >
			uint64_t make_hash(const k_t &s) const;

			/// @brief Creates a has from the string.
			/// @param s The string.
			/// @return The hash.
			uint64_t make_hash(const char *s) const;

			/// @brief Compares two keys.
			/// @tparam k_t The key type.
			/// @param a A key.
			/// @param b Another key.
			/// @return True if the keys are exactly equal.
			template < typename k_t >
			bool kcmp(const k_t &a, const k_t &b) const;

			/// @brief Compares two strings.
			/// @param a A string.
			/// @param b Another string.
			/// @return True if the strings are exactly equal.
			bool kcmp(const char *a, const char *b) const;

			/// @brief Deletes memory of specified node and sub-nodes.
			/// @param n The node to delete.
			void free_node(node *n);

			/// @brief Walks the entire tree depth-first order and calls the provided function.
			/// @tparam fn_t The function to call at each node in the tree, taking the value type as input.
			/// @param fn The function.
			/// @param n The node to traverse.
			template < typename fn_t >
			void traverse(fn_t &fn, node *n);

		public:
			/// @brief Initializes search tree.
			search_tree( void );

			/// @brief Frees memory in search tree.
			~search_tree( void );

			/// @brief Returns an existing value if key exists, or adds a new key-value pair if it does not.
			/// @param key The name of the job class derivative that will be used to call the instance function.
			/// @param value The instance function that allocates memory for the specified job class derivative.
			type_t *add(const key_t &key, const type_t &value);

			/// @brief Returns a pointer to the value pointed to by the key.
			/// @param key The key under which a value has been stored.
			/// @return Pointer to the value pointed to by key. Null if the key is not detected.
			type_t *get(const key_t &key);

			/// @brief Returns a pointer to the value pointed to by the key.
			/// @param key The key under which a value has been stored.
			/// @return Pointer to the value pointed to by key. Null if the key is not detected.
			const type_t *get(const key_t &key) const;

			/// @brief Removes a key-value pair from the search tree. If the key does not exist the function does not do anything (i.e. safe to remove a non-existing key).
			/// @param key The key under which a value has been stored.
			void remove(const key_t &key);

			/// @brief Walks the entire tree depth-first order and calls the provided function.
			/// @tparam fn_t The function to call at each node in the tree, taking the value type as input.
			/// @param fn The function.
			template < typename fn_t >
			void traverse(fn_t &fn);
		};

		/// @brief A helper class that will ensure a working in-house RTTI when inheriting from job classes.
		/// @tparam self_t The derived class.
		/// @tparam self_type_name_t A class containing a function, name(), which returns the name of the current class as a string. This string can be used to instantiate this class via string.
		/// @tparam base_t The class derived from (must itself be a derivative of jobs_internal::inherit).
		/// @note For RTTI to work, the user must indirectly inherit jobs and tasks via the inherit class.
		template < typename self_t, typename self_type_name_t, typename base_t = cc0::job >
		class inherit : public base_t
		{
		private:
			static const bool m_registered;

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
			static jobs_internal::rtti *instance( void );

			/// @brief Returns the type ID of the object instance (polymorphic).
			/// @return The type ID of the object instance.
			uint64_t object_id( void ) const;

			/// @brief Returns the custom name of the object instance (polymorphic).
			/// @return The custom name of the object instance.
			const char *object_name( void ) const;

			/// @brief Returns the custom name of the class.
			/// @return The custom name of the class.
			static const char *type_name( void );

			/// @brief Determines if the class has been registered so that it can be instantiated via a string.
			/// @return True if registered. False if there is another class registered under the same name.
			static bool is_registered( void );
		};

		/// @brief An ease-of-use typedef for the function pointer signature used to instantiate job class derivatives.
		typedef rtti* (*instance_fn)(void);
	}

	/// @brief A job. Updates itself and its children using custom code that can be inserted via overloading virtual functions within the class.
	CC0_JOBS_DERIVE(job, jobs_internal::rtti)
	{
	private:
		/// @brief Shared data used by automatic reference counting.
		struct shared
		{
			uint64_t watchers;
			bool     deleted;
		};

		/// @brief A generic callback for member functions of all derivatives of jobs.
		class base_callback
		{
		public:
			/// @brief Virtual destructor.
			virtual ~base_callback( void ) {}

			/// @brief Calls the stored callback.
			/// @param sender The sender.
			virtual void operator()(job &sender) = 0;
		};

		/// @brief A generic member function callback.
		template < typename job_t >
		class event_callback : public base_callback
		{
		private:
			job_t *m_self;
			void (job_t::*m_memfn)(job&);

		public:
			/// @brief Constructor.
			/// @param self  The object to call the member function from.
			/// @param fn The member function.
			event_callback(job_t *self, void (job_t::*fn)(job&));

			/// @brief Call the callback.
			/// @param sender The sender.
			void operator()(job &sender);
		};

		/// @brief A memory managed callback. Automatically deletes on destruction.
		class callback
		{
		private:
			base_callback *m_callback;
		
		public:
			/// @brief Constructs the object. Everything null.
			callback( void );

			/// @brief Destroys the object. Deletes the memory.
			~callback( void );

			/// @brief Allocates memory for a callback.
			/// @param fn The callback function.
			template < typename job_t >
			void set(job_t *self, void (job_t::*fn)(job&));

			/// @brief Calls the stored callback.
			/// @param sender The sender.
			void operator()(job &sender);
		};

	public:
		/// @brief Safely references a job. Will yield null if the referenced job has been destroyed.
		/// @tparam job_t The base class of the reference. Defaults to the fundamental job.
		template < typename job_t = cc0::job >
		class ref
		{
			friend class job;

		private:
			job_t  *m_job;
			shared *m_shared;
		
		public:
			/// @brief Default constructor.
			ref( void );

			/// @brief Initializes the reference.
			/// @tparam job2_t Type of the job pointer to reference.
			/// @param p The job to reference.
			template < typename job2_t >
			explicit ref(job2_t *p = nullptr);

			/// @brief Copy a reference.
			/// @tparam job2_t Type of the job pointer to reference.
			/// @param r The reference to copy.
			template < typename job2_t >
			ref(const ref<job2_t> &r);

			/// @brief Move a reference.
			/// @tparam job2_t Type of the job pointer to reference.
			/// @param r The reference to move.
			template < typename job2_t >
			ref(ref<job2_t> &&r);
			
			/// @brief Destroys the reference.
			~ref( void );

			/// @brief Copy a reference.
			/// @tparam job2_t Type of the job pointer to reference.
			/// @param r The reference to copy.
			/// @return The modified object.
			template < typename job2_t >
			ref &operator=(const ref<job2_t> &r);

			/// @brief Move a reference.
			/// @tparam job2_t Type of the job pointer to reference.
			/// @param r The reference to move.
			/// @return The modified object.
			template < typename job2_t >
			ref &operator=(ref<job2_t> &&r);

			/// @brief References a new job.
			/// @tparam job2_t Type of the job pointer to reference.
			/// @param p The job to reference.
			template < typename job2_t >
			void set_ref(job2_t *p);

			/// @brief Releases the reference. Deletes the metadata memory if this is the last reference referencing it. 
			void release( void );

			/// @brief Returns the job.
			/// @return The job. Null if the job has been deleted.
			job_t *get_job( void );

			/// @brief Returns the job.
			/// @return The job. Null if the job has been deleted.
			const job_t *get_job( void ) const;

			/// @brief Casts the current reference to another.
			/// @tparam job2_t Type of the job pointer to cast to.
			template < typename job2_t >
			ref<job2_t> cast( void );

			/// @brief Returns the job.
			/// @return The job. Null if the job has been deleted.
			job_t *operator->( void );

			/// @brief Returns the job.
			/// @return The job. Null if the job has been deleted.
			const job_t *operator->( void ) const;
		};

		/// @brief A search query containing a number of filters executed in sequence on the subject's children.
		/// @note Filters are alternative, meaning if a job fits any of the filters, then the job is selected.
		class query
		{
		public:
			class results;
			
			/// @brief A single result from a search query.
			class result
			{
				friend class results;

			private:
				job::ref<>   m_job;
				result      *m_next;
				result     **m_prev;
			
			public:
				/// @brief Constructs a result.
				/// @param j The job to store in the result.
				result(job &j);

				/// @brief Destroys the next result in the chain.
				~result( void );
				
				/// @brief Returns the job stored in the result.
				/// @return The job stored in the result.
				job::ref<> &get_job( void );

				/// @brief Returns the job stored in the result.
				/// @return The job stored in the result.
				const job::ref<> &get_job( void ) const;

				/// @brief Returns the next result in the chain.
				/// @return The next result in the chain.
				result *get_next( void );

				/// @brief Returns the next result in the chain.
				/// @return The next result in the chain.
				const result *get_next( void ) const;

				/// @brief Deletes memory and removes result from chain of results.
				/// @return The next result in the chain.
				result *remove( void );
			};

			/// @brief All results from a search query.
			class results
			{
			private:
				struct join_node
				{
					job     *value;
					int64_t  count;
				};

			private:
				/// @brief Inserts all results into a search tree. Duplicates are not stored, but instead tracked via a counter.
				/// @param t The tree to insert the results into.
				/// @param r The results to insert into the tree.
				static void insert_and_increment(jobs_internal::search_tree<join_node,const job*> &t, results &r);

				/// @brief Decrements all occurrences in the result list from the tree.
				/// @param t The tree to decrement the results of.
				/// @param r The results to decrement from the tree.
				static void remove_and_decrement(jobs_internal::search_tree<join_node,const job*> &t, results &r);

			private:
				result  *m_first;
				result **m_end;
			
			public:
				/// @brief Default constructor.
				results( void );

				/// @brief Deletes memory.
				~results( void );

				/// @brief Moves results from one list to another.
				/// @param r The list to move the results from.
				results(results &&r);

				/// @brief Moves results from one list to another.
				/// @param r The list to move the results from.
				/// @return A reference to self.
				results &operator=(results &&r);

				/// @brief Returns the first result in the collection of results.
				/// @return The first result in the collection of results.
				result *get_results( void );

				/// @brief Returns the first result in the collection of results.
				/// @return The first result in the collection of results.
				const result *get_results( void ) const;

				/// @brief Counts the number of results.
				/// @return The number of results.
				uint64_t count_results( void ) const;

				/// @brief Adds a job to the results.
				/// @param j The job to add.
				void add_result(job &j);

				/// @brief Applies an additional filter to the current results and returns the results.
				/// @tparam query_t The query type.
				/// @param q The query object.
				/// @return The results of the filter.
				template < typename query_t >
				results filter_results(const query_t &q);

				/// @brief Applies an additional filter to the current results and returns the results.
				/// @tparam query_t The query type.
				/// @return The results of the filter.
				template < typename query_t >
				results filter_results( void );

				/// @brief Applies an additional filter to the current results and returns the results.
				/// @param q The query object.
				/// @return The results of the filter.
				results filter_results(const query &q);

			public:
				/// @brief Takes two result lists and joins them together in a resulting list if an entry exists in both input lists (set intersection).
				/// @param a One result list.
				/// @param b Another result list.
				/// @return The resulting list.
				/// @note Any potential duplicates are only added once in the resulting list.
				static results join_and(results &a, results &b);

				/// @brief Takes two result lists and joins all entries in both input lists together into a resulting list (set union).
				/// @param a One result list.
				/// @param b Another result list.
				/// @return The resulting list.
				/// @note Any potential duplicates are only added once in the resulting list.
				static results join_or(results &a, results &b);

				/// @brief Adds the entries in the left input results list into a resulting list if the entry does not exist in the right input results list (set difference).
				/// @param l Left result list.
				/// @param r Right result list.
				/// @return The resulting list.
				/// @note Any potential duplicates are only added once in the resulting list.
				static results join_sub(results &l, results &r);

				/// @brief Takes two result lists and joins them together in a resulting list if an entry is unique to only one of the input lists (set symmetric difference).
				/// @param a One result list.
				/// @param b Another result list.
				/// @return The resulting list.
				/// @note Any potential duplicates are only added once in the resulting list.
				static results join_xor(results &a, results &b);
			};

		public:
			/// @brief Frees data in query.
			virtual ~query( void );
			
			/// @brief Applies a user-defined test (filter) to a provided job.
			/// @param j The job to apply the filter to.
			/// @return True if the provided job matches the criteria.
			/// @note Override this with custom behavior to create custom filters for searches. 
			virtual bool operator()(const job &j) const;
		};

	private:
		static jobs_internal::search_tree<jobs_internal::instance_fn> m_products;

	private:
		job                                  *m_parent;                  // A pointer for the job's parent.
		job                                  *m_sibling;                 // A pointer to the first sibling of potentially many sibling jobs.
		job                                  *m_child;                   // A pointer to the first child of potentially many child jobs.
		uint64_t                              m_job_id;                  // The unique ID of this job.
		uint64_t                              m_sleep_ns;                // The amount of time, in nanoseconds, that the job should currently sleep for.
		uint64_t                              m_created_at_ns;           // The timestamp at which the job was created.
		uint64_t                              m_existed_for_ns;          // The number of nanoseconds that the job has existed for.
		uint64_t                              m_active_for_ns;           // The number of nanoseconds that the job has been active for.
		uint64_t                              m_existed_tick_count;      // The number of ticks that the job has existed for.
		uint64_t                              m_active_tick_count;       // The number of ticks that the job has been active for.
		uint64_t                              m_time_scale;              // The current scale that the input duration is subjected to.
		uint64_t                              m_min_duration_ns;         // The minimum allowed duration to be passed to the job during a tick.
		uint64_t                              m_max_duration_ns;         // The maximum allowed duration to be passed to the job during a tick.
		uint64_t                              m_accumulated_duration_ns; // The accumulated time between job runs.
		uint64_t                              m_max_ticks_per_cycle;     // TODO IMPL
		jobs_internal::search_tree<callback>  m_event_callbacks;         // Holds the callbacks to be triggered when a particular event is sent to the job.
		shared                               *m_shared;                  // Holds information about references to this job.
		bool                                  m_enabled;                 // Indicates that the job is allowed to run.
		bool                                  m_kill;                    // Indicates that the user has marked the job for termination.
		bool                                  m_waiting;                 // Indicates that the job is being attempted to run despite the provided input time duration is less than the allowed duration minimum.
		bool                                  m_tick_lock;               // Indicates that recursive operations are prevented from manually ticking the job.
	
	private:
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
		/// @param duration_ns The time elapsed.
		void tick_children(uint64_t duration_ns);

		/// @brief Pass an event to this job from a sender.
		/// @param event The event string.
		/// @param sender The sender.
		void get_notified(const char *event, job &sender);

		/// @brief Scales a time.
		/// @param time The time to scale.
		/// @param time_scale The fixed-point scale, shifted by 32 bits.
		/// @return The scaled time.
		static uint64_t scale_time(uint64_t time, uint64_t time_scale);

		/// @brief Gets the accumulated time scale of all parents.
		/// @return The accumulated time scale.
		uint64_t get_parent_time_scale( void ) const;

	protected:
		/// @brief Called when ticking the job, before the children are ticked.
		/// @param duration_ns The time elapsed. User defined.
		/// @note There is no default behavior. This must be overloaded.
		virtual void on_tick(uint64_t duration_ns);
		
		/// @brief Called when ticking the job, after the children have been ticked.
		/// @param duration_ns The time elapsed. User defined.
		/// @note There is no default behavior. This must be overloaded.
		virtual void on_tock(uint64_t duration_ns);

		/// @brief Called immediately when the job is created.
		/// @note There is no default behavior. This must be overloaded.
		virtual void on_birth( void );

		/// @brief Called immediately when the job is killed.
		/// @note There is no default behavior. This must be overloaded.
		virtual void on_death( void );

	public:
		/// @brief Initializes the job.
		job( void );

		/// @brief Destroys the children, then the next sibling.
		~job( void );

		/// @brief Calls on_tick, ticks all children, and on_tock.
		/// @param duration_ns The time elapsed.
		void cycle(uint64_t duration_ns);
		
		/// @brief Queues the job for destruction. The 'on_death' function is called immediately (if the object is active), but the memory for the job may not be freed immediately.
		/// @note Kills all children first, then kills the parent job. The 'death' function is only called if the job is active.
		void kill( void );

		/// @brief Kills all children of the node, envoking the 'on_death' function in each.
		void kill_children( void );
		
		/// @brief Lets the job sleep for a given amount of time. If the job is already sleeping only the difference in time is added to the sleep duration (if time is larger than the current sleep duration).
		/// @param duration_ns The amount of time to sleep.
		void sleep_for(uint64_t duration_ns);

		/// @brief Disables sleep.
		void wake( void );

		/// @brief Adds an event for the job to listen and respond to.
		/// @tparam job_t The sub-class the callback method is declared in.
		/// @param event The event to listen to.
		/// @param callback The member function to call when this job receives the event.
		template < typename job_t >
		void listen(const char *event, void (job_t::*callback)(job&));

		/// @brief Stops listening to the named event.
		/// @param event The event to stop listening to.
		void ignore(const char *event);

		/// @brief Adds a child to the job's list of children.
		/// @tparam job_t The type of the child to add to the job.
		/// @return A pointer to the job that was added of the type of the added job.
		template < typename job_t >
		job_t *add_child( void );

		/// @brief Adds a child to the job's list of children.
		/// @param name The class name of the child to add to the job. This must correspond to the name registered when declaring the job.
		/// @return A pointer to the job that was added of the type of a generic job. Null if the name has not been declared properly using CC0_JOBS_NEW or CC0_JOBS_DERIVE.
		/// @sa CC0_JOBS_NEW
		/// @sa CC0_JOBS_DERIVE
		job *add_child(const char *type_name);

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

		/// @brief Checks if the job is currently waiting, i.e. active, but currently skipping tick/tock due to duration interval settings.
		/// @return True if the job currently has skipped a tick/tock cycle.
		bool is_waiting( void ) const;

		/// @brief Checks if the job is currently ready, i.e. not waiting.
		/// @return True if the job currently has not skipped a tick/tock cycle.
		bool is_ready( void ) const;

		/// @brief Returns the job ID.
		/// @return The job ID.
		uint64_t get_job_id( void ) const;

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

		/// @brief Notify the target job of an event.
		/// @param event The event string.
		/// @param target The target of the event.
		/// @note Nothing will happen if the job is not active. 
		void notify(const char *event, job &target);

		/// @brief Returns a safe reference that will automatically turn null if the job is deleted.
		/// @return The reference.
		ref<> get_ref( void );

		/// @brief Gets the accumulated time the job has existed for.
		/// @return The accumulated time (in nanoseconds) the job has existed for.
		uint64_t get_existed_for( void ) const;

		/// @brief Gets the accumulated time the job has been active for.
		/// @return The accumulated time (in nanoseconds) the job has been active for.
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
		void set_local_time_scale(float time_scale);

		/// @brief Gets the current time scale for this job.
		/// @return The current time scale for this job.
		float get_local_time_scale( void ) const;

		/// @brief Sets the time scale for this job on a global level.
		/// @param time_scale The desired global time scaling.
		void set_global_time_scale(float time_scale);

		/// @brief Gets the current time scale for this job on a global level.
		/// @return The current time scale for this job on a global level.
		float get_global_time_scale( void ) const;

		/// @brief Returns the time according to the job, which may have had its time dilated due to time scaling.
		/// @return The time according to the job.
		uint64_t get_local_time_ns( void ) const;

		/// @brief Returns the timestamp when this job was first created.
		/// @return The timestamp when this job was first created.
		/// @note Time scaling can make this vary between jobs in a counter-intuitive way.
		uint64_t created_at_ns( void ) const;

		/// @brief Applies a query.
		/// @return A list of results containing children matching the query.
		query::results filter_children(const query &q);

		/// @brief Applies a query.
		/// @tparam query_t The type of the query. Can be a class overloading the () operator taking a const-ref job and returning bool, or a function taking a const-ref job and returning bool.
		/// @param q The query.
		/// @return A list of results containing children matching the query.
		template < typename query_t >
		query::results filter_children(const query_t &q);

		/// @brief Applies a query.
		/// @tparam query_t The type of the query. Can be a class overloading the () operator taking a const-ref job and returning bool, or a function taking a const-ref job and returning bool.
		/// @return A list of results containing children matching the query.
		template < typename query_t >
		query::results filter_children( void );

		/// @brief Returns a query result including all children.
		/// @tparam job_t The children of the job matching job type.
		/// @return A query result including all children.
		query::results get_children( void );

		/// @brief Returns a query filtering out children not of the requested job type.
		/// @tparam job_t The children of the job matching job type.
		/// @return A list of results filtering out children not of the requested job type.
		template < typename job_t >
		query::results get_children( void );

		/// @brief Adds the job class derivative to the factory in order for it to be able to be instantiated later.
		/// @tparam job_t The type of the job class derivative.
		/// @param name The name of the job class derivative as a string that can later be used to call the instantiation function.
		/// @return True if the name and job were successfully registered. False if another job has already been registered under the provided name.
		/// @note Do not use this function directly. Instead use CC0_JOBS_NEW or CC0_JOBS_DERIVE.
		/// @sa CC0_JOBS_NEW
		/// @sa CC0_JOBS_DERIVE
		template < typename job_t >
		static bool register_job(const char *type_name);

		/// @brief Traverses the child tree and counts the number of child jobs present under this parent.
		/// @return The number of child jobs present under this parent.
		uint64_t count_children( void ) const;

		/// @brief Traverses the entire sub-tree and counts the number of decendant jobs under this parent.
		/// @return The number of decendant jobs present under this parent.
		uint64_t count_decendants( void ) const;

		/// @brief Forces duration input in tick function to fall within a given range.
		/// @param min_duration_ns The smallest duration that can be handled. If a smaller duration is passed to the tick function, the job will sleep.
		/// @param max_duration_ns The largest duration that can be handled. If a larger duration is passed to the tick function, the duration will be clipped to the max duration.
		/// @sa limit_tick_rate
		/// @note Does the same thing as limit_tick_rate, but works with durations.
		void limit_tick_interval(uint64_t min_duration_ns, uint64_t max_duration_ns);

		/// @brief Removes restrictions on duration input to tick function.
		/// @note Does the same thing as unlimit_tick_rate.
		/// @sa unlimit_tick_rate
		void unlimit_tick_interval( void );

		/// @brief Forces duration input in tick function to fall within a given range.
		/// @param min_ticks_per_sec The smallest amount of ticks per second that can be handled. If a smaller amount of ticks are suspected the duration is capped to match the minimum expected ticks per second.
		/// @param max_ticks_per_sec The largest amount of ticks per second that can be handled. If a larger amount of ticks are suspected the job will sleep.
		/// @sa limit_tick_duration_interval
		/// @note Does the same thing as lock_duration_interval, but works with frequency.
		void limit_tick_rate(uint64_t min_ticks_per_sec, uint64_t max_ticks_per_sec);

		/// @brief Removes restrictions on duration input to tick function.
		/// @note Does the same thing as unlock_duration_interval.
		/// @sa unlimit_tick_duration_interval
		void unlimit_tick_rate( void );

		/// @brief Returns the minimum allowed duration (ns) between ticks.
		/// @return The minimum allowed duration (ns) between ticks.
		uint64_t get_min_duration_ns( void ) const;

		/// @brief Returns the maximum allowed duration (ns) between ticks.
		/// @return The maximum allowed duration (ns) between ticks.
		uint64_t get_max_duration_ns( void ) const;

		/// @brief Returns the minimum allowed tick frequency per second.
		/// @return The minimum allowed tick frequency per second.
		uint64_t get_min_tick_per_sec( void ) const;

		/// @brief Returns the maximum allowed tick frequency per second.
		/// @return The maximum allowed tick frequency per second.
		uint64_t get_max_tick_per_sec( void ) const;

		/// @brief Returns true if the job is tick (rate) limited.
		/// @return True if the job is tick (rate) limited.
		bool is_tick_limited( void ) const;

		/// @brief Creates a new job of the given type.
		/// @param type_name The name of the type.
		/// @return The created job. Null if the type name is not registered, found, of if the type could not be converted into the base job class.
		static cc0::job *create_orphan(const char *type_name);

		/// @brief Determines if the job has any enabled children.
		/// @return True if there is at least one enabled child. False if there are no enabled children, or no children at all.
		bool has_enabled_children( void ) const;

		/// @brief Continues execution until the job no longer is enabled.
		/// @note This is the function that users want to trigger manually for root nodes as it will perform timing and continuously execute until the job, and its sub-jobs, are finished.
		void run( void );
	};
}

//
// rtti
//

template < typename type_t >
type_t *cc0::jobs_internal::rtti::cast( void )
{
	return reinterpret_cast<type_t*>(self(type_t::type_id()));
}

template < typename type_t >
const type_t *cc0::jobs_internal::rtti::cast( void ) const
{
	return reinterpret_cast<const type_t*>(self(type_t::type_id()));
}

//
// search_tree
//

template < typename type_t, typename key_t >
template < typename k_t >
uint64_t cc0::jobs_internal::search_tree<type_t,key_t>::make_hash(const k_t &k) const
{
	const uint8_t *K = reinterpret_cast<const uint8_t*>(&k);
	uint64_t sum = 0xcbf29ce484222325ULL;
	for (uint64_t i = 0; i < sizeof(k_t); ++i) {
		sum ^= uint64_t(K[i]);
		sum *= 0x100000001b3ULL;
	}
	return sum;
}

template < typename type_t, typename key_t >
uint64_t cc0::jobs_internal::search_tree<type_t,key_t>::make_hash(const char *s) const
{
	uint64_t sum = 0xcbf29ce484222325ULL;
	for (uint64_t i = 0; s[i] != 0; ++i) {
		sum ^= uint64_t(s[i]);
		sum *= 0x100000001b3ULL;
	}
	return sum;
}

template < typename type_t, typename key_t >
template < typename k_t >
bool cc0::jobs_internal::search_tree<type_t,key_t>::kcmp(const k_t &a, const k_t &b) const
{
	const uint8_t *A = reinterpret_cast<const uint8_t*>(a);
	const uint8_t *B = reinterpret_cast<const uint8_t*>(b);
	for (uint32_t i = 0; i < sizeof(k_t); ++i) {
		if (A[i] != B[i]) { return false; };
	}
	return true;
}

template < typename type_t, typename key_t >
bool cc0::jobs_internal::search_tree<type_t,key_t>::kcmp(const char *a, const char *b) const
{
	while (*a != 0) {
		if (*a != *b) { return false; }
		++a;
		++b;
	}
	return (*a == *b);
}

template < typename type_t, typename key_t >
void cc0::jobs_internal::search_tree<type_t,key_t>::free_node(cc0::jobs_internal::search_tree<type_t,key_t>::node *n)
{
	if (n != nullptr) {
		free_node(n->lte);
		free_node(n->gt);
		delete n;
	}
}

template < typename type_t, typename key_t >
template < typename fn_t >
void cc0::jobs_internal::search_tree<type_t,key_t>::traverse(fn_t &fn, cc0::jobs_internal::search_tree<type_t,key_t>::node *n)
{
	if (n != nullptr) {
		traverse(fn, n->lte);
		fn(n->value);
		traverse(fn, n->gt);
	}
}

template < typename type_t, typename key_t >
cc0::jobs_internal::search_tree<type_t,key_t>::search_tree( void ) : m_root(nullptr) 
{}

template < typename type_t, typename key_t >
cc0::jobs_internal::search_tree<type_t,key_t>::~search_tree( void )
{
	free_node(m_root);
}

template < typename type_t, typename key_t >
type_t *cc0::jobs_internal::search_tree<type_t,key_t>::add(const key_t &key, const type_t &value)
{
	const uint64_t hash = make_hash(key);
	node **n = &m_root;
	while (*n != nullptr) {
		if (hash <= (*n)->hash) {
			if (hash == (*n)->hash && kcmp(key, (*n)->key)) {
				return &((*n)->value);
			} else {
				n = &((*n)->lte);
			}
		} else {
			n = &((*n)->gt);
		}
	}
	*n = new node{ hash, key, nullptr, nullptr, value };
	return &((*n)->value);
}

template < typename type_t, typename key_t >
type_t *cc0::jobs_internal::search_tree<type_t,key_t>::get(const key_t &key)
{
	const uint64_t hash = make_hash(key);
	node *n = m_root;
	while (n != nullptr) {
		if (hash <= n->hash) {
			if (hash == n->hash && kcmp(key, n->key)) {
				return &(n->value);
			} else {
				n = n->lte;
			}
		} else {
			n = n->gt;
		}
	}
	return nullptr;
}

template < typename type_t, typename key_t >
const type_t *cc0::jobs_internal::search_tree<type_t,key_t>::get(const key_t &key) const
{
	const uint64_t hash = make_hash(key);
	const node *n = m_root;
	while (n != nullptr) {
		if (hash <= n->hash) {
			if (hash == n->hash && kcmp(key, n->key)) {
				return &(n->value);
			} else {
				n = n->lte;
			}
		} else {
			n = n->gt;
		}
	}
	return nullptr;
}

template < typename type_t, typename key_t >
void cc0::jobs_internal::search_tree<type_t,key_t>::remove(const key_t &key)
{	
	const uint64_t hash = make_hash(key);

	node *p = nullptr;
	node *n = m_root;
	uint64_t rel = 0;
	
	while (n != nullptr && n->hash != hash && !kcmp(n->key, key)) {
		p = n;
		if (hash <= n->hash) {
			n = n->lte;
			rel = 1;
		} else {
			n = n->gt;
			rel = 2;
		}
	}

	if (n != nullptr) {
		
		const uint64_t child_count = (n->gt ? 1 : 0) + (n->lte ? 1 : 0);

		if (child_count == 0) {
			if (rel == 1)      { p->lte = nullptr; }
			else if (rel == 2) { p->gt  = nullptr; }
			else               { m_root = nullptr; }
		} else if (child_count == 1) {
			node *&c = n->gt ? n->gt : n->lte;
			if (rel == 1)      { p->lte = c; }
			else if (rel == 2) { p->gt  = c; }
			else               { m_root = c; }
			c = nullptr;
		} else if (child_count == 2) {
			node *c = n->gt;
			while (c->lte != nullptr) {
				c = c->lte;
			}
			if (rel == 1)      { p->lte = c; }
			else if (rel == 2) { p->gt  = c; }
			else               { m_root = c; }
			c->lte = n->lte;
			c->gt  = n->gt;
			n->lte = nullptr;
			n->gt  = nullptr;
		}
		delete n;
	}
}

template < typename type_t, typename key_t >
template < typename fn_t >
void cc0::jobs_internal::search_tree<type_t,key_t>::traverse(fn_t &fn)
{
	traverse(fn, m_root);
}

//
// inherit
//

template < typename self_t, typename self_type_name_t, typename base_t >
const bool cc0::jobs_internal::inherit<self_t,self_type_name_t,base_t>::m_registered = cc0::job::register_job<self_t>(cc0::jobs_internal::inherit<self_t,self_type_name_t,base_t>::type_name());

template < typename self_t, typename self_type_name_t, typename base_t >
void *cc0::jobs_internal::inherit<self_t,self_type_name_t,base_t>::self(uint64_t type_id)
{
	return this->type_id() == type_id ? this : base_t::self(type_id);
}

template < typename self_t, typename self_type_name_t, typename base_t >
const void *cc0::jobs_internal::inherit<self_t,self_type_name_t,base_t>::self(uint64_t type_id) const
{
	return this->type_id() == type_id ? this : base_t::self(type_id);
}

template < typename self_t, typename self_type_name_t, typename base_t >
uint64_t cc0::jobs_internal::inherit<self_t,self_type_name_t,base_t>::type_id( void )
{
	static const uint64_t id = cc0::jobs_internal::new_uuid();
	return id;
}

template < typename self_t, typename self_type_name_t, typename base_t >
cc0::jobs_internal::rtti *cc0::jobs_internal::inherit<self_t,self_type_name_t,base_t>::instance( void )
{
	return new self_t;
}

template < typename self_t, typename self_type_name_t, typename base_t >
uint64_t cc0::jobs_internal::inherit<self_t,self_type_name_t,base_t>::object_id( void ) const
{
	return type_id();
}

template < typename self_t, typename self_type_name_t, typename base_t >
const char *cc0::jobs_internal::inherit<self_t,self_type_name_t,base_t>::object_name( void ) const
{
	return type_name();
}

template < typename self_t, typename self_type_name_t, typename base_t >
const char *cc0::jobs_internal::inherit<self_t,self_type_name_t,base_t>::type_name( void )
{
	return self_type_name_t::name();
}

template < typename self_t, typename self_type_name_t, typename base_t >
bool cc0::jobs_internal::inherit<self_t,self_type_name_t,base_t>::is_registered( void )
{
	return m_registered;
}

//
// event_callback
//

template < typename job_t >
cc0::job::event_callback<job_t>::event_callback(job_t *self, void (job_t::*fn)(cc0::job&)) : m_self(self), m_memfn(fn)
{}

template < typename job_t >
void cc0::job::event_callback<job_t>::operator()(cc0::job &sender)
{
	return (m_self->*m_memfn)(sender);
}

//
// callback
//

template < typename job_t >
void cc0::job::callback::set(job_t *self, void (job_t::*fn)(cc0::job&))
{
	delete m_callback;
	m_callback = new event_callback<job_t>(self, fn);
}

//
// ref
//

template < typename job_t >
cc0::job::ref<job_t>::ref( void ) : m_job(nullptr), m_shared(nullptr)
{}

template < typename job_t >
template < typename job2_t >
cc0::job::ref<job_t>::ref(job2_t *p) : m_job(p), m_shared(p != nullptr ? p->m_shared : nullptr)
{
	if (m_shared != nullptr) {
		++m_shared->watchers;
	}
}

template < typename job_t >
template < typename job2_t >
cc0::job::ref<job_t>::ref(const cc0::job::ref<job2_t> &r) : ref()
{
	set_ref(r.m_job);
}

template < typename job_t >
template < typename job2_t >
cc0::job::ref<job_t>::ref(cc0::job::ref<job2_t> &&r) : m_job(r.m_job), m_shared(r.m_shared)
{
	r.m_job = nullptr;
	r.m_shared = nullptr;
}

template < typename job_t >
cc0::job::ref<job_t>::~ref( void )
{
	release();
}

template < typename job_t >
template < typename job2_t >
cc0::job::ref<job_t> &cc0::job::ref<job_t>::operator=(const cc0::job::ref<job2_t> &r)
{
	if (this != &r) {
		set_ref(r.m_job);
	}
	return *this;
}

template < typename job_t >
template < typename job2_t >
cc0::job::ref<job_t> &cc0::job::ref<job_t>::operator=(cc0::job::ref<job2_t> &&r)
{
	if (this != &r) {
		set_ref(r.m_job);
		r.release();
	}
	return *this;
}

template < typename job_t >
template < typename job2_t >
void cc0::job::ref<job_t>::set_ref(job2_t *p)
{
	if (p != m_job) {
		release();
		if (p != nullptr) {
			m_job = p;
			m_shared = m_job->m_shared;
			++m_shared->watchers;
		}
	}
}

template < typename job_t >
void cc0::job::ref<job_t>::release( void )
{
	if (m_shared != nullptr) {
		--m_shared->watchers;
		if (m_shared->watchers == 0 && m_shared->deleted) {
			delete m_shared;
		}
	}
	m_job = nullptr;
	m_shared = nullptr;
}

template < typename job_t >
job_t *cc0::job::ref<job_t>::get_job( void )
{
	return m_shared != nullptr && !m_shared->deleted ? m_job : nullptr;
}

template < typename job_t >
const job_t *cc0::job::ref<job_t>::get_job( void ) const
{
	return m_shared != nullptr && !m_shared->deleted ? m_job : nullptr;
}

template < typename job_t >
template < typename job2_t >
cc0::job::ref<job2_t> cc0::job::ref<job_t>::cast( void )
{
	return ref<job2_t>(m_job->template cast<job2_t>());
}

template < typename job_t >
job_t *cc0::job::ref<job_t>::operator->( void )
{
	return m_job;
}

template < typename job_t >
const job_t *cc0::job::ref<job_t>::operator->( void ) const
{
	return m_job;
}

//
// results
//

template < typename query_t >
cc0::job::query::results cc0::job::query::results::filter_results(const query_t &q)
{
	results r;
	result *c = get_results();
	while (c != nullptr) {
		if (q(*c->get_job().get_job())) {
			r.add_result(*c->get_job().get_job());
		}
		c = c->get_next();
	}
	return r;
}

template < typename query_t >
cc0::job::query::results cc0::job::query::results::filter_results( void )
{
	return filter_results<query_t>(query_t());
}

//
// job
//

template < typename job_t >
void cc0::job::listen(const char *event, void (job_t::*fn)(cc0::job&))
{
	job_t *self = cast<job_t>();
	if (self != nullptr) {
		callback *c = m_event_callbacks.add(event, callback());
		c->set<job_t>(self, fn);
	}
}

template < typename job_t >
job_t *cc0::job::add_child( void )
{
	job_t *p = nullptr;
	if (!is_killed()) {
		p = new job_t;
		add_sibling(m_child, p);
		((job*)p)->m_created_at_ns = get_local_time_ns();
		((job*)p)->m_min_duration_ns = m_min_duration_ns;
		((job*)p)->m_max_duration_ns = m_max_duration_ns;
		p->on_birth();
	}
	return p;
}

template < typename query_t >
cc0::job::query::results cc0::job::filter_children(const query_t &q)
{
	return get_children().filter_results<query_t>(q);
}

template < typename query_t >
cc0::job::query::results cc0::job::filter_children( void )
{
	return filter_children<query_t>(query_t());
}

template < typename job_t >
cc0::job::query::results cc0::job::get_children( void )
{
	class type_filter : public job::query { public: bool operator()(const job &j) const { return j.cast<job_t>() != nullptr; } } q;
	return filter_children(q);
}

template < typename job_t >
bool cc0::job::register_job(const char *type_name)
{
	if (m_products.get(type_name) != nullptr) {
		return false;
	}
	m_products.add(type_name, job_t::instance);
	return true;
}

#endif
