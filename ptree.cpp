#include "ptree.h"

cc0::proc::ref::ref(cc0::proc *p) : m_proc(p), m_shared(p != nullptr ? p->m_shared : nullptr)
{
	if (m_shared != nullptr) {
		++m_shared->watchers;
	}
}

cc0::proc::ref::ref(const cc0::proc::ref &r) : ref()
{
	set_ref(r.m_proc);
}

cc0::proc::ref::ref(cc0::proc::ref &&r) : m_proc(r.m_proc), m_shared(r.m_shared)
{
	r.m_proc = nullptr;
	r.m_shared = nullptr;
}

cc0::proc::ref::~ref( void )
{
	release();
}

cc0::proc::ref &cc0::proc::ref::operator=(const cc0::proc::ref &r)
{
	if (this != &r) {
		set_ref(r.m_proc);
	}
	return *this;
}

cc0::proc::ref &cc0::proc::ref::operator=(cc0::proc::ref &&r)
{
	if (this != &r) {
		set_ref(r.m_proc);
		r.m_proc = nullptr;
		r.m_shared = nullptr;
	}
	return *this;
}

void cc0::proc::ref::set_ref(cc0::proc *p)
{
	if (p != m_proc) {
		release();
		if (p != nullptr) {
			m_proc = p;
			m_shared = m_proc->m_shared;
		}
	}
}

void cc0::proc::ref::release( void )
{
	if (m_shared != nullptr) {
		--m_shared->watchers;
		if (m_shared->watchers == 0 && m_shared->deleted) {
			delete m_shared;
		}
	}
	m_proc = nullptr;
	m_shared = nullptr;
}

cc0::proc *cc0::proc::ref::get_proc( void )
{
	return m_proc;
}

const cc0::proc *cc0::proc::ref::get_proc( void ) const
{
	return m_proc;
}

uint64_t cc0::proc::new_pid( void )
{
	static uint64_t pid = 0;
	return pid++;
}

void cc0::proc::set_deleted( void )
{
	m_shared->deleted = true;
}

void cc0::proc::add_sibling(cc0::proc *&loc, cc0::proc *p)
{
	if (loc == nullptr) {
		loc = p;
		loc->m_parent = this;
	} else {
		add_sibling(loc->m_sibling, p);
	}
}

void cc0::proc::delete_siblings(cc0::proc *&siblings)
{
	if (siblings != nullptr) {
		delete_siblings(siblings->m_sibling);
		delete siblings;
		siblings = nullptr;
	}
}

void cc0::proc::delete_children(cc0::proc *&children)
{
	if (children != nullptr) {
		delete_siblings(children->m_sibling);
		delete children;
		children = nullptr;
	}
}

void cc0::proc::delete_killed_children(cc0::proc *&child)
{
	if (child != nullptr) {
		delete_killed_children(child->m_sibling);
		if (child->is_killed()) {
			proc *sibling = child->m_sibling; // Save the next child in the list.
			child->m_sibling = nullptr;       // Set this to null to prevent recursive deletion of all subsequent siblings.
			delete child;                     // Delete the current child only.
			child = sibling;                  // Refer the current child node to the next child in the child list. This also repairs the linked list since the child pointer is a reference.
		}
	}
}

void cc0::proc::tick_children(uint64_t duration)
{
	for (cc0::proc *c = m_child; c != nullptr && is_active(); c = c->m_sibling) {
		c->tick(scale_time(duration, c->m_time_scale));
	}
}

uint64_t cc0::proc::scale_time(uint64_t time, uint64_t time_scale)
{
	return (time * time_scale) >> 16;
}

void cc0::proc::pre_tick(uint64_t duration)
{}

void cc0::proc::post_tick(uint64_t duration)
{}

void cc0::proc::death( void )
{}

void cc0::proc::handle_message(const char *event, proc *sender)
{}

cc0::proc::proc( void ) : m_parent(nullptr), m_sibling(nullptr), m_child(nullptr), m_pid(new_pid()), m_sleep(0), m_existed_for(0), m_active_for(0), m_existed_tick_count(0), m_active_tick_count(0), m_time_scale(1 << 16), m_shared(new shared{ 0, false }), m_enabled(true), m_kill(false)
{}

cc0::proc::~proc( void )
{
	delete m_child;
	m_child = nullptr;
	delete m_sibling;
	m_sibling = nullptr;

	set_deleted();
	if (m_shared->watchers == 0) {
		delete m_shared;
		m_shared = nullptr;
	}
}

void cc0::proc::tick(uint64_t duration)
{
	m_existed_for += duration;
	++m_existed_tick_count;
	if (is_sleeping()) {
		if (m_sleep <= duration) {
			m_sleep = 0;
			duration -= m_sleep;
		} else {
			m_sleep -= duration;
			duration = 0;
		}
	}

	if (is_active()) {
		m_active_for += duration;
		++m_active_tick_count;
		pre_tick(duration);
	}

	tick_children(duration);

	delete_killed_children(m_child);

	if (is_active()) {
		post_tick(duration);
	}
}

void cc0::proc::kill( void )
{
	if (is_active()) {
		m_enabled = false;
		m_kill = true;

		for (cc0::proc *c = m_child; c != nullptr; c = c->m_sibling) {
			c->kill();
		}

		delete m_child;
		m_child = nullptr;

		death();
	}
}

void cc0::proc::sleep(uint64_t time)
{
	m_sleep = m_sleep > time ? m_sleep : time;
}

void cc0::proc::wake( void )
{
	m_sleep = 0;
}

void cc0::proc::enable( void )
{
	m_enabled = true;
}

void cc0::proc::disable( void )
{
	m_enabled = false;
}

bool cc0::proc::is_killed( void ) const
{
	return m_kill;
}

bool cc0::proc::is_alive( void ) const
{
	return !is_killed();
}

bool cc0::proc::is_enabled( void ) const
{
	return !is_killed() && m_enabled;
}

bool cc0::proc::is_disabled( void ) const
{
	return !is_enabled();
}

bool cc0::proc::is_sleeping( void ) const
{
	return m_sleep > 0;
}

bool cc0::proc::is_awake( void ) const
{
	return !is_sleeping();
}

bool cc0::proc::is_active( void ) const
{
	return is_enabled() && !is_sleeping();
}

bool cc0::proc::is_inactive( void ) const
{
	return !is_active();
}

uint64_t cc0::proc::get_pid( void ) const
{
	return m_pid;
}

void cc0::proc::notify_parent(const char *event)
{
	if (m_parent != nullptr && is_active()) {
		m_parent->notify(event, this);
	}
}

void cc0::proc::notify_children(const char *event)
{
	for (cc0::proc *c = m_child; c != nullptr && is_active(); c = c->m_sibling) {
		c->notify(event, this);
	}
}

void cc0::proc::notify(const char *event, proc *sender)
{
	if (is_active()) {
		handle_message(event, sender);
	}
}

cc0::proc::ref cc0::proc::get_ref( void )
{
	return ref(this);
}

void cc0::ptree::pre_tick(uint64_t duration)
{
	if (get_child() == nullptr) {
		kill();
	}
}

uint64_t cc0::proc::get_existed_for( void ) const
{
	return m_existed_for;
}

uint64_t cc0::proc::get_active_for( void ) const
{
	return m_active_for;
}

uint64_t cc0::proc::get_existed_tick_count( void ) const
{
	return m_existed_tick_count;
}

uint64_t cc0::proc::get_active_tick_count( void ) const
{
	return m_active_tick_count;
}

cc0::proc *cc0::proc::get_parent( void )
{
	return m_parent;
}

const cc0::proc *cc0::proc::get_parent( void ) const
{
	return m_parent;
}

cc0::proc *cc0::proc::get_child( void )
{
	return m_child;
}

const cc0::proc *cc0::proc::get_child( void ) const
{
	return m_child;
}

cc0::proc *cc0::proc::get_sibling( void )
{
	return m_sibling;
}

const cc0::proc *cc0::proc::get_sibling( void ) const
{
	return m_sibling;
}

void cc0::proc::set_time_scale(float time_scale)
{
	m_time_scale = uint64_t(time_scale * (1 << 16));
}

float cc0::proc::get_time_scale( void ) const
{
	return m_time_scale / float(1<<16);
}
