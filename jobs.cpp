/// @file
/// @author github.com/SirJonthe
/// @date 2022, 2023
/// @copyright Public domain.
/// @license CC0 1.0

#include "jobs.h"

uint64_t cc0::jobs::new_uuid( void )
{
	static uint64_t uuid = 0;
	return uuid++;
}

void *cc0::jobs::internal::rtti::self(uint64_t type_id)
{
	return this->type_id() == type_id ? this : nullptr;
}

const void *cc0::jobs::internal::rtti::self(uint64_t type_id) const
{
	return this->type_id() == type_id ? this : nullptr;
}

uint64_t cc0::jobs::internal::rtti::type_id( void )
{
	static const uint64_t id = cc0::jobs::new_uuid();
	return id;
}

cc0::jobs::internal::rtti *cc0::jobs::internal::rtti::instance( void )
{
	return new rtti;
}

cc0::jobs::job::ref::ref(cc0::jobs::job *p) : m_job(p), m_shared(p != nullptr ? p->m_shared : nullptr)
{
	if (m_shared != nullptr) {
		++m_shared->watchers;
	}
}

cc0::jobs::job::ref::ref(const cc0::jobs::job::ref &r) : ref()
{
	set_ref(r.m_job);
}

cc0::jobs::job::ref::ref(cc0::jobs::job::ref &&r) : m_job(r.m_job), m_shared(r.m_shared)
{
	r.m_job = nullptr;
	r.m_shared = nullptr;
}

cc0::jobs::job::ref::~ref( void )
{
	release();
}

cc0::jobs::job::ref &cc0::jobs::job::ref::operator=(const cc0::jobs::job::ref &r)
{
	if (this != &r) {
		set_ref(r.m_job);
	}
	return *this;
}

cc0::jobs::job::ref &cc0::jobs::job::ref::operator=(cc0::jobs::job::ref &&r)
{
	if (this != &r) {
		set_ref(r.m_job);
		r.m_job = nullptr;
		r.m_shared = nullptr;
	}
	return *this;
}

void cc0::jobs::job::ref::set_ref(cc0::jobs::job *p)
{
	if (p != m_job) {
		release();
		if (p != nullptr) {
			m_job = p;
			m_shared = m_job->m_shared;
		}
	}
}

void cc0::jobs::job::ref::release( void )
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

cc0::jobs::job *cc0::jobs::job::ref::get_job( void )
{
	return m_job;
}

const cc0::jobs::job *cc0::jobs::job::ref::get_job( void ) const
{
	return m_job;
}

uint64_t cc0::jobs::job::factory::inventory::make_key(const char *s) const
{
	uint64_t k = 0;
	uint64_t i = 0;
	while (s[i] != 0) {
		k += uint64_t(s[i]) * (i+1);
		++i;
	}
	return k;
}

bool cc0::jobs::job::factory::inventory::str_cmp(const char *a, const char *b) const
{
	while (*a != 0) {
		if (*a != *b) { return false; }
		++a;
		++b;
	}
	return (*a == *b);
}

void cc0::jobs::job::factory::inventory::free_node(cc0::jobs::job::factory::inventory::node *n)
{
	if (n != nullptr) {
		free_node(n->lte);
		free_node(n->gt);
		delete n;
	}
}

cc0::jobs::job::factory::inventory::inventory( void ) : m_root(nullptr) 
{}

cc0::jobs::job::factory::inventory::~inventory( void )
{
	free_node(m_root);
}

void cc0::jobs::job::factory::inventory::add_job(const char *name, cc0::jobs::instance_fn instance)
{
	node *nn = new node{ make_key(name), name, instance, nullptr, nullptr };
	node *&n = m_root;
	while (n != nullptr) {
		if (nn->key <= n->key) {
			n = n->lte;
		} else {
			n = n->gt;
		}
	}
	n = nn;
}

cc0::jobs::internal::rtti *cc0::jobs::job::factory::inventory::instance_job(const char *name)
{
	uint64_t key = make_key(name);
	node *n = m_root;
	while (n != nullptr) {
		if (n->key == key && str_cmp(n->name, name)) {
			break;
		}
		if (key > n->key) {
			n = n->gt;
		} else {
			n = n->lte;
		}
	}
	return n != nullptr ? n->instance() : nullptr;
}

cc0::jobs::job::factory::inventory cc0::jobs::job::factory::m_products = cc0::jobs::job::factory::inventory();

cc0::jobs::job *cc0::jobs::job::factory::instance_job(const char *name)
{
	return m_products.instance_job(name)->cast<cc0::jobs::job>();
}

bool cc0::jobs::job::query::filter::do_execute(const cc0::jobs::job &j) const
{
	return true;
}

cc0::jobs::job::query::filter::filter( void ) : m_or(nullptr), m_and(nullptr)
{}

cc0::jobs::job::query::filter::~filter( void )
{
	delete m_or;
	delete m_and;
}

bool cc0::jobs::job::query::filter::operator()(const job &j) const
{
	return (do_execute(j) || (m_or != nullptr ? (*m_or)(j) : false)) && (m_and != nullptr ? (*m_and)(j) : true);
}

cc0::jobs::job::query::result::result(cc0::jobs::job &j, cc0::jobs::job::query::result **prev) : m_job(j.get_ref())
{
	m_prev = prev;
	if (prev != nullptr && *prev != nullptr) {
		m_next = (*prev)->m_next;
		(*prev)->m_next = this;
	} else {
		m_next = nullptr;
	}
}

cc0::jobs::job::query::result::~result( void )
{
	delete m_next;
	m_next = nullptr;
}

cc0::jobs::job::ref cc0::jobs::job::query::result::get_job( void )
{
	return m_job;
}

cc0::jobs::job::query::result *cc0::jobs::job::query::result::get_next( void )
{
	return m_next;
}

cc0::jobs::job::query::result *cc0::jobs::job::query::result::get_prev( void )
{
	return m_prev != nullptr ? *m_prev : nullptr;
}

cc0::jobs::job::query::result *cc0::jobs::job::query::result::remove( void )
{
	cc0::jobs::job::query::result *next = m_next;
	if (m_prev != nullptr) {
		*m_prev = m_next;
	}
	m_next = nullptr;
	delete this;
	return next;
}

cc0::jobs::job::query::results::results( void ) : m_first(nullptr)
{}

cc0::jobs::job::query::results::~results( void )
{
	delete m_first;
}

cc0::jobs::job::query::result *cc0::jobs::job::query::results::get_results( void )
{
	return m_first;
}

cc0::jobs::job::query::query(cc0::jobs::job *subject) : m_subject(subject->get_ref()), m_filter(nullptr)
{}

cc0::jobs::job::query::~query( void )
{
	delete m_filter;
}

cc0::jobs::job::query::results cc0::jobs::job::query::execute( void )
{
	return cc0::jobs::job::query::results();
}

uint64_t cc0::jobs::job::new_pid( void )
{
	return cc0::jobs::new_uuid();
}

void cc0::jobs::job::set_deleted( void )
{
	m_shared->deleted = true;
}

void cc0::jobs::job::add_sibling(cc0::jobs::job *&loc, cc0::jobs::job *p)
{
	if (loc == nullptr) {
		loc = p;
		loc->m_parent = this;
	} else {
		add_sibling(loc->m_sibling, p);
	}
}

void cc0::jobs::job::delete_siblings(cc0::jobs::job *&siblings)
{
	if (siblings != nullptr) {
		delete_siblings(siblings->m_sibling);
		delete siblings;
		siblings = nullptr;
	}
}

void cc0::jobs::job::delete_children(cc0::jobs::job *&children)
{
	if (children != nullptr) {
		delete_siblings(children->m_sibling);
		delete children;
		children = nullptr;
	}
}

void cc0::jobs::job::delete_killed_children(cc0::jobs::job *&child)
{
	if (child != nullptr) {
		delete_killed_children(child->m_sibling);
		if (child->is_killed()) {
			job *sibling = child->m_sibling; // Save the next child in the list.
			child->m_sibling = nullptr;      // Set this to null to prevent recursive deletion of all subsequent siblings.
			delete child;                    // Delete the current child only.
			child = sibling;                 // Refer the current child node to the next child in the child list. This also repairs the linked list since the child pointer is a reference.
		}
	}
}

void cc0::jobs::job::tick_children(uint64_t duration)
{
	for (cc0::jobs::job *c = m_child; c != nullptr && is_active(); c = c->m_sibling) {
		c->tick(duration);
	}
}

uint64_t cc0::jobs::job::scale_time(uint64_t time, uint64_t time_scale)
{
	return (time * time_scale) >> 16;
}

void cc0::jobs::job::on_tick(uint64_t duration)
{}

void cc0::jobs::job::on_tock(uint64_t duration)
{}

void cc0::jobs::job::on_birth( void )
{}

void cc0::jobs::job::on_death( void )
{}

void cc0::jobs::job::on_message(const char *event, cc0::jobs::job *sender)
{}

cc0::jobs::job::job( void ) :
	m_parent(nullptr), m_sibling(nullptr), m_child(nullptr),
	m_pid(new_pid()),
	m_sleep(0),
	m_existed_for(0), m_active_for(0), m_existed_tick_count(0), m_active_tick_count(0),
	m_time_scale(1 << 16),
	m_shared(new shared{ 0, false }),
	m_enabled(true), m_kill(false)
{}

cc0::jobs::job::~job( void )
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

void cc0::jobs::job::tick(uint64_t duration)
{
	duration = scale_time(duration, m_time_scale);

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
		on_tick(duration);
	}

	tick_children(duration);

	delete_killed_children(m_child);

	if (is_active()) {
		on_tock(duration);
	}
}

void cc0::jobs::job::kill( void )
{
	if (is_alive()) {
		m_enabled = false;
		m_kill    = true;

		kill_children();

		delete m_child;
		m_child = nullptr;

		on_death();
	}
}

void cc0::jobs::job::kill_children( void )
{
	if (is_alive()) {
		for (cc0::jobs::job *c = m_child; c != nullptr; c = c->m_sibling) {
			c->kill();
		}
	}
}

void cc0::jobs::job::sleep(uint64_t time)
{
	m_sleep = m_sleep > time ? m_sleep : time;
}

void cc0::jobs::job::wake( void )
{
	m_sleep = 0;
}

void cc0::jobs::job::enable( void )
{
	m_enabled = true;
}

void cc0::jobs::job::disable( void )
{
	m_enabled = false;
}

bool cc0::jobs::job::is_killed( void ) const
{
	return m_kill;
}

bool cc0::jobs::job::is_alive( void ) const
{
	return !is_killed();
}

bool cc0::jobs::job::is_enabled( void ) const
{
	return !is_killed() && m_enabled;
}

bool cc0::jobs::job::is_disabled( void ) const
{
	return !is_enabled();
}

bool cc0::jobs::job::is_sleeping( void ) const
{
	return m_sleep > 0;
}

bool cc0::jobs::job::is_awake( void ) const
{
	return !is_sleeping();
}

bool cc0::jobs::job::is_active( void ) const
{
	return is_enabled() && !is_sleeping();
}

bool cc0::jobs::job::is_inactive( void ) const
{
	return !is_active();
}

uint64_t cc0::jobs::job::get_pid( void ) const
{
	return m_pid;
}

void cc0::jobs::job::notify_parent(const char *event)
{
	if (m_parent != nullptr && is_active()) {
		m_parent->notify(event, this);
	}
}

void cc0::jobs::job::notify_children(const char *event)
{
	for (cc0::jobs::job *c = m_child; c != nullptr && is_active(); c = c->m_sibling) {
		c->notify(event, this);
	}
}

void cc0::jobs::job::notify_group(const char *event, cc0::jobs::job::query::results &group)
{
	query::result *r = group.get_results();
	while (r != nullptr && is_active()) {
		r->get_job().get_job()->notify(event, this);
		r = r->get_next();
	}
}

void cc0::jobs::job::notify(const char *event, cc0::jobs::job *sender)
{
	if (is_active()) {
		on_message(event, sender);
	}
}

cc0::jobs::job::ref cc0::jobs::job::get_ref( void )
{
	return ref(this);
}

uint64_t cc0::jobs::job::get_existed_for( void ) const
{
	return m_existed_for;
}

uint64_t cc0::jobs::job::get_active_for( void ) const
{
	return m_active_for;
}

uint64_t cc0::jobs::job::get_existed_tick_count( void ) const
{
	return m_existed_tick_count;
}

uint64_t cc0::jobs::job::get_active_tick_count( void ) const
{
	return m_active_tick_count;
}

cc0::jobs::job *cc0::jobs::job::get_parent( void )
{
	return m_parent;
}

const cc0::jobs::job *cc0::jobs::job::get_parent( void ) const
{
	return m_parent;
}

cc0::jobs::job *cc0::jobs::job::get_child( void )
{
	return m_child;
}

const cc0::jobs::job *cc0::jobs::job::get_child( void ) const
{
	return m_child;
}

cc0::jobs::job *cc0::jobs::job::get_sibling( void )
{
	return m_sibling;
}

const cc0::jobs::job *cc0::jobs::job::get_sibling( void ) const
{
	return m_sibling;
}

cc0::jobs::job *cc0::jobs::job::get_root( void )
{
	cc0::jobs::job *r = this;
	while (r->m_parent != nullptr) {
		r = r->m_parent;
	}
	return r;
}

const cc0::jobs::job *cc0::jobs::job::get_root( void ) const
{
	const cc0::jobs::job *r = this;
	while (r->m_parent != nullptr) {
		r = r->m_parent;
	}
	return r;
}

void cc0::jobs::job::set_time_scale(float time_scale)
{
	m_time_scale = uint64_t(time_scale * (1 << 16));
}

float cc0::jobs::job::get_time_scale( void ) const
{
	return m_time_scale / float(1<<16);
}

cc0::jobs::job::query cc0::jobs::job::search( void )
{
	return query(this);
}

cc0::jobs::job *cc0::jobs::job::instance_job(const char *name)
{
	return factory::instance_job(name);
}

void cc0::jobs::jobs::on_tick(uint64_t duration)
{
	if (get_child() == nullptr) {
		kill();
	}
}

cc0::jobs::jobs::jobs( void ) : inherit(), m_min_duration(0), m_max_duration(0), m_duration(m_min_duration)
{}

cc0::jobs::jobs::jobs(uint64_t min_ticks_per_sec, uint64_t max_ticks_per_sec) : inherit(), m_min_duration(1000 / min_ticks_per_sec), m_max_duration(1000 / max_ticks_per_sec), m_duration(m_min_duration)
{}
