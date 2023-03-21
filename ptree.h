#ifndef PTREE_H_INCLUDED__
#define PTREE_H_INCLUDED__

#include <cstdint>

namespace cc0
{
	/// @brief A process. Updates itself and its children using custom code that can be inserted via overloading virtual functions within the class.
	class proc
	{
	private:
		/// @brief Shared data used by automatic reference counting.
		struct shared
		{
			uint64_t watchers;
			bool     deleted;
		};

	public:
		/// @brief Safely references a process. Will yield null if the referenced process has been destroyed.
		class ref
		{
			friend class proc;

		private:
			proc   *m_proc;
			shared *m_shared;
		
		public:
			/// @brief Initializes the reference.
			/// @param p The process to reference.
			explicit ref(proc *p = nullptr);

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

			/// @brief References a new process.
			/// @param p The process to reference.
			void set_ref(proc *p);

			/// @brief Releases the reference. Deletes the metadata memory if this is the last reference referencing it. 
			void release( void );

			/// @brief Returns the process.
			/// @return The process. Null if the process has been deleted.
			proc *get_proc( void );

			/// @brief Returns the process.
			/// @return The process. Null if the process has been deleted.
			const proc *get_proc( void ) const;
		};

	private:
		proc     *m_parent;
		proc     *m_sibling;
		proc     *m_child;
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
		/// @brief Generates a new, unique process ID. Just increments a counter.
		/// @return The new process ID.
		static uint64_t new_pid( void );

		/// @brief  Tells the shared object that the referenced object has been deleted.
		void set_deleted( void );

		/// @brief Adds a process as a sibling. If specified sibling location is null, then the process is added there, otherwise it recursively traverses to the next sibling location.
		/// @param loc The sibling location.
		/// @param p The process to add as a sibling.
		void add_sibling(proc *&loc, proc *p);

		/// @brief Deletes all siblings.
		/// @param siblings The first sibling in the list of siblings.
		void delete_siblings(proc *&siblings);

		/// @brief Deletes all children and grandchildren.
		/// @param children The first child in the list of children.
		void delete_children(proc *&children);

		/// @brief Deletes all children that have been marked as killed.
		/// @param child The current child in the list.
		void delete_killed_children(proc *&child);

		/// @brief Ticks children.
		/// @param duration The time elapsed.
		void tick_children(uint64_t duration);

		/// @brief Scales a time.
		/// @param time The time to scale.
		/// @param time_scale The fixed-point scale, shifted by 32 bits.
		/// @return The scaled time.
		static uint64_t scale_time(uint64_t time, uint64_t time_scale);
	
	protected:
		/// @brief Called when ticking the process, before the children are ticked.
		/// @param duration The time elapsed.
		/// @note There is no default behavior. This must be overloaded.
		virtual void pre_tick(uint64_t duration);
		
		/// @brief Called when ticking the process, after the children have been ticked.
		/// @param duration The time elapsed.
		/// @note There is no default behavior. This must be overloaded.
		virtual void post_tick(uint64_t duration);

		/// @brief Called immediately when the process is killed.
		/// @note There is no default behavior. This must be overloaded.
		virtual void death( void );

		/// @brief The function that processes an incoming message via one of the notify functions.
		/// @param event The event string.
		/// @param sender The sender.
		/// @note There is no default behavior. This must be overloaded.
		virtual void handle_message(const char *event, proc *sender);

	public:
		/// @brief Initializes the process.
		proc( void );

		/// @brief Destroys the children, then the next sibling.
		~proc( void );

		/// @brief Calls pre_tick, ticks all children, and post_tick.
		/// @param duration The time elapsed.
		void tick(uint64_t duration);
		
		/// @brief Queues the process for destruction. The 'death' function is called immediately (if the object is active), but the memory for the process may not be freed immediately.
		/// @note Kills all children first, then kills the parent process. The 'death' function is only called if the process is active.
		void kill( void );
		
		/// @brief Lets the process sleep for a given amount of time. If the process is already sleeping only the difference in time is added to the sleep duration (if time is larger than the current sleep duration).
		/// @param time The amount of time to sleep.
		void sleep(uint64_t time);

		/// @brief Disables sleep.
		void wake( void );

		/// @brief Adds an event for the process to listen and respond to.
		// void listen(const char *event_id);

		/// @brief Adds a child to the process' list of children.
		/// @tparam proc_t The type of the child to add to the process.
		/// @return The process that was added.
		template < typename proc_t >
		proc_t &add_child( void );

		/// @brief Enables the process, allowing it to tick and call the death function.
		void enable( void );

		/// @brief Disables the process, disabling ticking and death function.
		void disable( void );

		/// @brief Checks if the process has been killed.
		/// @return True if the process has been killed.
		bool is_killed( void ) const;

		/// @brief Checks if the process is not killed.
		/// @return True if the process is not killed.
		bool is_alive( void ) const;

		/// @brief Checks if the process is enabled.
		/// @return True if the process is enabled.
		/// @note Killing an object automatically sets the process to disabled.
		bool is_enabled( void ) const;

		/// @brief Checks if the process is disabled.
		/// @return True if the process is disabled.
		/// @note Killing an object automatically sets the process to disabled.
		bool is_disabled( void ) const;
		
		/// @brief Checks if the process is sleeping.
		/// @return True if the process is sleeping.
		bool is_sleeping( void ) const;

		/// @brief Checks if the process is awake.
		/// @return True if the process is awake.
		bool is_awake( void ) const;
		
		/// @brief Checks if the process is currently active.
		/// @return True if the process is neither killed, disabled, or asleep.
		bool is_active( void ) const;

		/// @brief Checks if the process is currently inactive.
		/// @return True if the process is either killed, disabled, or asleep.
		bool is_inactive( void ) const;

		/// @brief Returns the process ID.
		/// @return The process ID.
		uint64_t get_pid( void ) const;

		/// @brief Notifies the parent of an event.
		/// @param event The event string.
		/// @note Nothing will happen if the process is not active.
		void notify_parent(const char *event);

		/// @brief Notify all children of an event.
		/// @param event The event string.
		/// @note Nothing will happen if the process is not active.
		void notify_children(const char *event);

		/// @brief Notify the process of an event.
		/// @param event The event string.
		/// @param sender The process sending the event.
		/// @note Nothing will happen if the process is not active. 
		void notify(const char *event, proc *sender);

		/// @brief Returns a safe reference that will automatically turn null if the process is deleted.
		/// @return The reference.
		ref get_ref( void );

		/// @brief Gets the accumulated time the process has existed for.
		/// @return The accumulated time the process has existed for.
		uint64_t get_existed_for( void ) const;

		/// @brief Gets the accumulated time the process has been active for.
		/// @return The accumulated time the process has been active for.
		uint64_t get_active_for( void ) const;

		/// @brief Gets the accumulated number of ticks the process has existed for.
		/// @return The accumulated number of ticks the process has existed for.
		uint64_t get_existed_tick_count( void ) const;

		/// @brief Gets the accumulated number of ticks the process has been active for.
		/// @return The accumulated number of ticks the process has been active for.
		uint64_t get_active_tick_count( void ) const;

		/// @brief Returns the parent process.
		/// @return The parent process.
		proc *get_parent( void );

		/// @brief Returns the parent process.
		/// @return The parent process.
		const proc *get_parent( void ) const;

		/// @brief Returns the first child process.
		/// @return The first child process.
		proc *get_child( void );

		/// @brief Returns the first child process.
		/// @return The first child process.
		const proc *get_child( void ) const;

		/// @brief Returns the next sibling process.
		/// @return The next sibling process.
		proc *get_sibling( void );

		/// @brief Returns the next sibling process.
		/// @return The next sibling process.
		const proc *get_sibling( void ) const;

		/// @brief Sets the time scale for this process.
		/// @param time_scale The desired time scaling.
		void set_time_scale(float time_scale);

		/// @brief Gets the current time scale for this process.
		/// @return The current time scale for this process.
		float get_time_scale( void ) const;
	};

	/// @brief The root of the process tree. Kills execution if it has no children.
	class ptree : public proc
	{
	private:
		uint64_t m_min_duration;
		uint64_t m_max_duration;
		uint64_t m_duration;

	protected:
		/// @brief Kills children if it has no children.
		/// @param duration The time elapsed since last tick.
		void pre_tick(uint64_t duration);

		// TODO IMPL
		/// @brief Sleeps the tree.
		/// @param duration The time elapsed since last tick.
		//void post_tick(uint64_t duration);
	
	public:
		// TODO DOC
		/// @brief  Default constructor. No tick limits.
		//ptree( void );
		
		/// TODO DOC
		/// @brief Constructs the tree with tick limits.
		/// @param min_ticks_per_sec The minimum number of ticks that will be performed per second. If the processes do not hit the target, the durations are clipped to the 1000 / min_ticks_per_sec.
		/// @param max_ticks_per_sec The maximum number of ticks that will be performed per second. If the processes exceed the target the tree sleeps.
		//ptree(uint64_t min_ticks_per_sec, uint64_t max_ticks_per_sec);

		// TODO DOC
		/// @brief 
		/// @param  
		//void tick( void );
	};
}

template < typename proc_t >
proc_t &cc0::proc::add_child( void )
{
	proc_t *p = nullptr;
	if (!is_killed()) {
		p = new proc_t;
		add_sibling(m_child, p);
	}
	return *p;
}

#endif
