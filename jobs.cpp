/// @file
/// @author github.com/SirJonthe
/// @date 2022, 2023
/// @copyright Public domain.
/// @license CC0 1.0

#include <thread>
#include "jobs.h"

//
// global
//

#define NS_PER_SEC 1000000000ULL

uint64_t cc0::jobs::internal::new_uuid( void )
{
	static uint64_t uuid = 0;
	return uuid++;
}

//
// rtti
//

void *cc0::jobs::internal::rtti::self(uint64_t type_id)
{
	return this->type_id() == type_id ? this : nullptr;
}

const void *cc0::jobs::internal::rtti::self(uint64_t type_id) const
{
	return this->type_id() == type_id ? this : nullptr;
}

cc0::jobs::internal::rtti::~rtti( void )
{}

uint64_t cc0::jobs::internal::rtti::type_id( void )
{
	static const uint64_t id = cc0::jobs::internal::new_uuid();
	return id;
}

cc0::jobs::internal::rtti *cc0::jobs::internal::rtti::instance( void )
{
	return new rtti;
}

uint64_t cc0::jobs::internal::rtti::object_id( void ) const
{
	return type_id();
}

const char *cc0::jobs::internal::rtti::object_name( void ) const
{
	return type_name();
}

const char *cc0::jobs::internal::rtti::type_name( void )
{
	return "rtti";
}

//
// callback
//

cc0::jobs::job::callback::callback( void ) : m_callback(nullptr)
{}

cc0::jobs::job::callback::~callback( void )
{
	delete m_callback;
}

void cc0::jobs::job::callback::operator()(cc0::jobs::job &sender)
{
	if (m_callback != nullptr) {
		(*m_callback)(sender);
	}
}

//
// result
//

cc0::jobs::job::query::result::result(cc0::jobs::job &j) : m_job(j.get_ref()), m_next(nullptr)
{}

cc0::jobs::job::query::result::~result( void )
{
	delete m_next;
	m_next = nullptr;
}

cc0::jobs::job::ref<> &cc0::jobs::job::query::result::get_job( void )
{
	return m_job;
}

const cc0::jobs::job::ref<> &cc0::jobs::job::query::result::get_job( void ) const
{
	return m_job;
}

cc0::jobs::job::query::result *cc0::jobs::job::query::result::get_next( void )
{
	return m_next;
}

const cc0::jobs::job::query::result *cc0::jobs::job::query::result::get_next( void ) const
{
	return m_next;
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

//
// results
//

void cc0::jobs::job::query::results::insert_and_increment(cc0::jobs::internal::search_tree<join_node,const cc0::jobs::job*> &t, cc0::jobs::job::query::results &r)
{
	result *i = r.get_results();
	while (i != nullptr) {
		join_node *n = t.get(i->get_job().get_job());
		if (n != nullptr) {
			++n->count;
		} else {
			t.add(i->get_job().get_job(), join_node{ i->get_job().get_job(), 1 });
		}
		i = i->get_next();
	}
}

void cc0::jobs::job::query::results::remove_and_decrement(cc0::jobs::internal::search_tree<join_node,const cc0::jobs::job*> &t, cc0::jobs::job::query::results &r)
{
	result *i = r.get_results();
	while (i != nullptr) {
		join_node *n = t.get(i->get_job().get_job());
		if (n != nullptr) {
			--n->count;
		}
		i = i->get_next();
	}
}

cc0::jobs::job::query::results::results( void ) : m_first(nullptr), m_end(&m_first)
{}

cc0::jobs::job::query::results::~results( void )
{
	delete m_first;
}

cc0::jobs::job::query::results::results(cc0::jobs::job::query::results &&r) : results()
{
	m_first = r.m_first;
	m_end = r.m_end;

	r.m_first = nullptr;
	r.m_end = &r.m_first;
}

cc0::jobs::job::query::results &cc0::jobs::job::query::results::operator=(cc0::jobs::job::query::results &&r)
{
	if (this != &r) {
		delete m_first;
		
		m_first = r.m_first;
		m_end   = r.m_end;
		
		r.m_first = nullptr;
		r.m_end = &r.m_first;
	}
	
	return *this;
}

cc0::jobs::job::query::result *cc0::jobs::job::query::results::get_results( void )
{
	return m_first;
}

const cc0::jobs::job::query::result *cc0::jobs::job::query::results::get_results( void ) const
{
	return m_first;
}

uint64_t cc0::jobs::job::query::results::count_results( void ) const
{
	const result *r = m_first;
	uint64_t c = 0;
	while (r != nullptr) {
		++c;
		r = r->get_next();
	}
	return c;
}

void cc0::jobs::job::query::results::add_result(cc0::jobs::job &j)
{
	*m_end = new query::result(j);
	m_end = &(*m_end)->m_next;
}

cc0::jobs::job::query::results cc0::jobs::job::query::results::filter_results(const cc0::jobs::job::query &q)
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

cc0::jobs::job::query::results cc0::jobs::job::query::results::join_and(cc0::jobs::job::query::results &a, cc0::jobs::job::query::results &b)
{
	internal::search_tree<join_node,const job*> t;
	insert_and_increment(t, a);
	insert_and_increment(t, b);
	results res;
	struct accum
	{
		results *r;
		accum(results *res) : r(res) {}
		void operator()(join_node &n) {
			if (n.count >= 2) {
				r->add_result(*n.value);
			}
		}
	} fn(&res);
	t.traverse(fn);
	return res;
}

cc0::jobs::job::query::results cc0::jobs::job::query::results::join_or(cc0::jobs::job::query::results &a, cc0::jobs::job::query::results &b)
{
	internal::search_tree<join_node,const job*> t;
	insert_and_increment(t, a);
	insert_and_increment(t, b);
	results res;
	struct accum
	{
		results *r;
		accum(results *res) : r(res) {}
		void operator()(join_node &n) {
			r->add_result(*n.value);
		}
	} fn(&res);
	t.traverse(fn);
	return res;
}

cc0::jobs::job::query::results cc0::jobs::job::query::results::join_sub(cc0::jobs::job::query::results &l, cc0::jobs::job::query::results &r)
{
	internal::search_tree<join_node,const job*> t;
	insert_and_increment(t, l);
	remove_and_decrement(t, r);
	results res;
	struct accum
	{
		results *r;
		accum(results *res) : r(res) {}
		void operator()(join_node &n) {
			if (n.count == 1) {
				r->add_result(*n.value);
			}
		}
	} fn(&res);
	t.traverse(fn);
	return res;
}

cc0::jobs::job::query::results cc0::jobs::job::query::results::join_xor(cc0::jobs::job::query::results &a, cc0::jobs::job::query::results &b)
{
	internal::search_tree<join_node,const job*> t;
	insert_and_increment(t, a);
	insert_and_increment(t, b);
	results res;
	struct accum
	{
		results *r;
		accum(results *res) : r(res) {}
		void operator()(join_node &n) {
			if (n.count == 1) {
				r->add_result(*n.value);
			}
		}
	} fn(&res);
	t.traverse(fn);
	return res;
}

//
// query
//

cc0::jobs::job::query::~query( void )
{}

bool cc0::jobs::job::query::operator()(const cc0::jobs::job &j) const
{
	return true;
}

//
// job
//

cc0::jobs::internal::search_tree<cc0::jobs::instance_fn> cc0::jobs::job::m_products = cc0::jobs::internal::search_tree<cc0::jobs::instance_fn>();

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

void cc0::jobs::job::tick_children(uint64_t duration_ns)
{
	for (cc0::jobs::job *c = m_child; c != nullptr && is_active(); c = c->m_sibling) {
		c->tick(duration_ns);
	}
}

void cc0::jobs::job::get_notified(const char *event, cc0::jobs::job &sender)
{
	if (is_active()) {
		callback *c = m_event_callbacks.get(event);
		if (c != nullptr) {
			(*c)(sender);
		}
	}
}

uint64_t cc0::jobs::job::scale_time(uint64_t time, uint64_t time_scale)
{
	return (time * time_scale) >> 16;
}

uint64_t cc0::jobs::job::adjust_duration(uint64_t duration_ns) const
{
	duration_ns = scale_time(duration_ns + m_duration_ns, m_time_scale);
	return duration_ns < m_max_duration_ns ? duration_ns : m_max_duration_ns;
}

void cc0::jobs::job::on_tick(uint64_t duration)
{}

void cc0::jobs::job::on_tock(uint64_t duration)
{}

void cc0::jobs::job::on_birth( void )
{}

void cc0::jobs::job::on_death( void )
{}

cc0::jobs::job::job( void ) :
	m_parent(nullptr), m_sibling(nullptr), m_child(nullptr),
	m_job_id(cc0::jobs::internal::new_uuid()),
	m_sleep_ns(0),
	m_existed_for_ns(0), m_active_for_ns(0), m_existed_tick_count(0), m_active_tick_count(0),
	m_min_duration_ns(0), m_max_duration_ns(0), m_duration_ns(m_min_duration_ns),
	m_time_scale(1 << 16),
	m_event_callbacks(),
	m_shared(new shared{ 0, false }),
	m_enabled(true), m_kill(false), m_tick_lock(false)
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

void cc0::jobs::job::tick(uint64_t duration_ns)
{
	if (!m_tick_lock) {
		m_tick_lock = true;

		duration_ns = adjust_duration(duration_ns);
		
		m_existed_for_ns += duration_ns;
		++m_existed_tick_count;

		if (is_sleeping()) {
			// TODO: Unsure if m_sleep_ns needs to be scaled or not...
			if (m_sleep_ns <= duration_ns) {
				m_sleep_ns = 0;
				duration_ns -= m_sleep_ns;
			} else {
				m_sleep_ns -= duration_ns;
				duration_ns = 0;
			}
		}

		if (duration_ns < m_min_duration_ns) {
			m_duration_ns += duration_ns;
			m_tick_lock = false;
			return;
		}

		if (is_active()) {
			m_active_for_ns += duration_ns;
			++m_active_tick_count;
			on_tick(duration_ns);
		}

		tick_children(duration_ns);

		delete_killed_children(m_child);

		if (is_active()) {
			on_tock(duration_ns);
		}

		m_duration_ns = 0;
		m_tick_lock = false;
	}
}

void cc0::jobs::job::kill( void )
{
	if (is_alive()) {
		kill_children();

		delete m_child;
		m_child = nullptr;

		on_death();

		m_enabled = false;
		m_kill    = true;
	}
}

void cc0::jobs::job::kill_children( void )
{
	for (cc0::jobs::job *c = m_child; c != nullptr; c = c->m_sibling) {
		c->kill();
	}
}

void cc0::jobs::job::sleep_for(uint64_t duration_ns)
{
	m_sleep_ns = m_sleep_ns > duration_ns ? m_sleep_ns : duration_ns;
}

void cc0::jobs::job::wake( void )
{
	m_sleep_ns = 0;
}

void cc0::jobs::job::ignore(const char *event)
{
	m_event_callbacks.remove(event);
}

cc0::jobs::job *cc0::jobs::job::add_child(const char *type_name)
{
	cc0::jobs::job *p = nullptr;
	if (!is_killed()) {
		p = create_orphan(type_name);
		if (p != nullptr) {
			add_sibling(m_child, p);
			p->m_min_duration_ns = m_min_duration_ns;
			p->m_max_duration_ns = m_max_duration_ns;
			p->on_birth();
		}
	}
	return p;
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
	return m_sleep_ns > 0;
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

uint64_t cc0::jobs::job::get_job_id( void ) const
{
	return m_job_id;
}

void cc0::jobs::job::notify_parent(const char *event)
{
	if (m_parent != nullptr && is_active()) {
		notify(event, *m_parent);
	}
}

void cc0::jobs::job::notify_children(const char *event)
{
	for (cc0::jobs::job *c = m_child; c != nullptr && is_active(); c = c->m_sibling) {
		notify(event, *c);
	}
}

void cc0::jobs::job::notify_group(const char *event, cc0::jobs::job::query::results &group)
{
	query::result *r = group.get_results();
	while (r != nullptr && is_active()) {
		notify(event, *r->get_job().get_job());
		r = r->get_next();
	}
}

void cc0::jobs::job::notify(const char *event, cc0::jobs::job &target)
{
	target.get_notified(event, *this);
}

cc0::jobs::job::ref<> cc0::jobs::job::get_ref( void )
{
	return ref<job>(this);
}

uint64_t cc0::jobs::job::get_existed_for( void ) const
{
	return m_existed_for_ns;
}

uint64_t cc0::jobs::job::get_active_for( void ) const
{
	return m_active_for_ns;
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
	m_time_scale = uint64_t(double(time_scale) * double(1 << 16));
}

float cc0::jobs::job::get_time_scale( void ) const
{
	return float(m_time_scale / double(1 << 16));
}

cc0::jobs::job::query::results cc0::jobs::job::filter_children(const cc0::jobs::job::query &q)
{
	return get_children().filter_results(q);
}

cc0::jobs::job::query::results cc0::jobs::job::get_children( void )
{
	query::results r;
	job *c = get_child();
	while (c != nullptr) {
		r.add_result(*c);
		c = c->get_sibling();
	}
	return r;
}

uint64_t cc0::jobs::job::count_children( void ) const
{
	const job *n = get_child();
	uint64_t c = 0;
	while (n != nullptr) {
		++c;
		n = n->get_sibling();
	}
	return c;
}

uint64_t cc0::jobs::job::count_decendants( void ) const
{
	const job *n = get_child();
	uint64_t c = 0;
	while (n != nullptr) {
		c += (n->count_decendants() + 1);
		n = n->get_sibling();
	}
	return c;
}

void cc0::jobs::job::limit_tick_interval(uint64_t min_duration_ns, uint64_t max_duration_ns)
{
	m_min_duration_ns = min_duration_ns < max_duration_ns ? min_duration_ns : max_duration_ns;
	m_max_duration_ns = min_duration_ns > max_duration_ns ? min_duration_ns : max_duration_ns;
}

void cc0::jobs::job::unlimit_tick_interval( void )
{
	m_min_duration_ns = 0;
	m_max_duration_ns = 0;
}

void cc0::jobs::job::limit_tick_rate(uint64_t min_ticks_per_sec, uint64_t max_ticks_per_sec)
{
	limit_tick_interval(NS_PER_SEC / max_ticks_per_sec, NS_PER_SEC / min_ticks_per_sec);
}

void cc0::jobs::job::unlimit_tick_rate( void )
{
	unlimit_tick_interval();
}

uint64_t cc0::jobs::job::get_min_duration_ns( void ) const
{
	return m_min_duration_ns;
}

uint64_t cc0::jobs::job::get_max_duration_ns( void ) const
{
	return m_max_duration_ns;
}

uint64_t cc0::jobs::job::get_min_tick_per_sec( void ) const
{
	return NS_PER_SEC / m_max_duration_ns;
}

uint64_t cc0::jobs::job::get_max_tick_per_sec( void ) const
{
	return NS_PER_SEC / m_min_duration_ns;
}

bool cc0::jobs::job::is_tick_limited( void ) const
{
	return m_min_duration_ns != 0 || m_max_duration_ns != 0;
}

cc0::jobs::job *cc0::jobs::job::create_orphan(const char *type_name)
{
	cc0::jobs::job *j = nullptr;
	instance_fn *i = m_products.get(type_name);
	if (i != nullptr) {
		cc0::jobs::internal::rtti *r = (*i)();
		if (r != nullptr) {
			j = r->cast<cc0::jobs::job>();
			if (j == nullptr) {
				delete r;
			}
			return j;
		}
	}
	return j;
}

bool cc0::jobs::job::has_enabled_children( void ) const
{
	const cc0::jobs::job *c = get_child();
	while (c != nullptr && c->is_disabled()) {
		c = c->get_sibling();
	}
	return c != nullptr;
}

//
// global
//

void cc0::jobs::run(cc0::jobs::job &root)
{
	uint64_t duration_ns = root.get_min_tick_per_sec() > 0 ? root.get_min_tick_per_sec() : 1;

	while (root.is_enabled()) {
		
		const uint64_t start_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();

		root.tick(duration_ns);

		duration_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count() - start_ns;

		if (duration_ns < root.get_min_duration_ns()) {
			std::this_thread::sleep_for(std::chrono::nanoseconds(root.get_min_duration_ns() - duration_ns));
			duration_ns = root.get_min_duration_ns();
		}
	}
}
