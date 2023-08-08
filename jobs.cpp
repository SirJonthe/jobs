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

uint64_t cc0::jobs_internal::new_uuid( void )
{
	static uint64_t uuid = 1; // Never allocate UUID=0 as we reserve that.
	return uuid++;
}

//
// rtti
//

void *cc0::jobs_internal::rtti::self(uint64_t type_id)
{
	return this->type_id() == type_id ? this : nullptr;
}

const void *cc0::jobs_internal::rtti::self(uint64_t type_id) const
{
	return this->type_id() == type_id ? this : nullptr;
}

cc0::jobs_internal::rtti::~rtti( void )
{}

uint64_t cc0::jobs_internal::rtti::type_id( void )
{
	static const uint64_t id = cc0::jobs_internal::new_uuid();
	return id;
}

cc0::jobs_internal::rtti *cc0::jobs_internal::rtti::instance( void )
{
	return new rtti;
}

uint64_t cc0::jobs_internal::rtti::object_id( void ) const
{
	return type_id();
}

const char *cc0::jobs_internal::rtti::object_name( void ) const
{
	return type_name();
}

const char *cc0::jobs_internal::rtti::type_name( void )
{
	return "rtti";
}

//
// fn_callback
//

cc0::job::fn_callback::fn_callback(void (*fn)(cc0::job&)) : m_fn(fn)
{}

void cc0::job::fn_callback::operator()(cc0::job &sender)
{
	if (m_fn != nullptr) {
		m_fn(sender);
	}
}

//
// callback
//

cc0::job::callback::callback( void ) : m_callback(nullptr)
{}

cc0::job::callback::~callback( void )
{
	delete m_callback;
}

void cc0::job::callback::set(void (*fn)(job&))
{
	delete m_callback;
	m_callback = new fn_callback(fn);
}

void cc0::job::callback::operator()(cc0::job &sender)
{
	if (m_callback != nullptr) {
		(*m_callback)(sender);
	}
}

//
// result
//

cc0::job::query::result::result(cc0::job &j) : m_job(j.get_ref()), m_next(nullptr)
{}

cc0::job::query::result::~result( void )
{
	delete m_next;
	m_next = nullptr;
}

cc0::job::ref<> &cc0::job::query::result::get_job( void )
{
	return m_job;
}

const cc0::job::ref<> &cc0::job::query::result::get_job( void ) const
{
	return m_job;
}

cc0::job::query::result *cc0::job::query::result::get_next( void )
{
	return m_next;
}

const cc0::job::query::result *cc0::job::query::result::get_next( void ) const
{
	return m_next;
}

cc0::job::query::result *cc0::job::query::result::remove( void )
{
	cc0::job::query::result *next = m_next;
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

void cc0::job::query::results::insert_and_increment(cc0::jobs_internal::search_tree<join_node,const cc0::job*> &t, cc0::job::query::results &r)
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

void cc0::job::query::results::remove_and_decrement(cc0::jobs_internal::search_tree<join_node,const cc0::job*> &t, cc0::job::query::results &r)
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

cc0::job::query::results::results( void ) : m_first(nullptr), m_end(&m_first)
{}

cc0::job::query::results::~results( void )
{
	delete m_first;
}

cc0::job::query::results::results(cc0::job::query::results &&r) : results()
{
	m_first = r.m_first;
	m_end = r.m_end;

	r.m_first = nullptr;
	r.m_end = &r.m_first;
}

cc0::job::query::results &cc0::job::query::results::operator=(cc0::job::query::results &&r)
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

cc0::job::query::result *cc0::job::query::results::get_results( void )
{
	return m_first;
}

const cc0::job::query::result *cc0::job::query::results::get_results( void ) const
{
	return m_first;
}

uint64_t cc0::job::query::results::count_results( void ) const
{
	const result *r = m_first;
	uint64_t c = 0;
	while (r != nullptr) {
		++c;
		r = r->get_next();
	}
	return c;
}

void cc0::job::query::results::add_result(cc0::job &j)
{
	*m_end = new query::result(j);
	m_end = &(*m_end)->m_next;
}

cc0::job::query::results cc0::job::query::results::filter_results(const cc0::job::query &q)
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

cc0::job::query::results cc0::job::query::results::join_and(cc0::job::query::results &a, cc0::job::query::results &b)
{
	cc0::jobs_internal::search_tree<join_node,const job*> t;
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

cc0::job::query::results cc0::job::query::results::join_or(cc0::job::query::results &a, cc0::job::query::results &b)
{
	cc0::jobs_internal::search_tree<join_node,const job*> t;
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

cc0::job::query::results cc0::job::query::results::join_sub(cc0::job::query::results &l, cc0::job::query::results &r)
{
	cc0::jobs_internal::search_tree<join_node,const job*> t;
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

cc0::job::query::results cc0::job::query::results::join_xor(cc0::job::query::results &a, cc0::job::query::results &b)
{
	cc0::jobs_internal::search_tree<join_node,const job*> t;
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

cc0::job::query::~query( void )
{}

bool cc0::job::query::operator()(const cc0::job &j) const
{
	return true;
}

//
// job
//

cc0::jobs_internal::search_tree<cc0::jobs_internal::instance_fn> cc0::job::m_products = cc0::jobs_internal::search_tree<cc0::jobs_internal::instance_fn>();

void cc0::job::set_deleted( void )
{
	m_shared->deleted = true;
}

void cc0::job::add_sibling(cc0::job *&loc, cc0::job *p)
{
	cc0::job *old_loc = loc;
	loc = p;
	loc->m_parent = this;
	loc->m_sibling = old_loc;
}

void cc0::job::delete_siblings(cc0::job *&siblings)
{
	if (siblings != nullptr) {
		delete_siblings(siblings->m_sibling);
		delete siblings;
		siblings = nullptr;
	}
}

void cc0::job::delete_children(cc0::job *&children)
{
	if (children != nullptr) {
		delete_siblings(children->m_sibling);
		delete children;
		children = nullptr;
	}
}

void cc0::job::delete_killed_children(cc0::job *&child)
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

void cc0::job::tick_children(uint64_t duration_ns)
{
	for (cc0::job *c = m_child; c != nullptr && is_active(); c = c->m_sibling) {
		c->cycle(duration_ns);
	}
}

void cc0::job::get_notified(const char *event, cc0::job &sender)
{
	if (is_active()) {
		callback_tree *t = m_event_callbacks.get(0);
		if (t != nullptr) {
			callback *c = t->get(event);
			if (c != nullptr) {
				(*c)(sender);
			}
		}
		t = m_event_callbacks.get(sender.get_job_id());
		if (t != nullptr) {
			callback *c = t->get(event);
			if (c != nullptr) {
				(*c)(sender);
			}
		}
	}
}

uint64_t cc0::job::scale_time(uint64_t time, uint64_t time_scale)
{
	return (time * time_scale) >> 16ULL;
}

uint64_t cc0::job::get_parent_time_scale( void ) const
{
	return m_parent != nullptr ? ((m_parent->m_time_scale * m_parent->get_parent_time_scale()) >> 16ULL) : (1ULL << 16ULL);
}

void cc0::job::on_tick(uint64_t duration)
{}

void cc0::job::on_tock(uint64_t duration)
{}

void cc0::job::on_birth( void )
{}

void cc0::job::on_death( void )
{}

cc0::job::job( void ) :
	m_parent(nullptr), m_sibling(nullptr), m_child(nullptr),
	m_job_id(cc0::jobs_internal::new_uuid()),
	m_sleep_ns(0),
	m_created_at_ns(0),
	m_existed_for_ns(0), m_active_for_ns(0), m_existed_tick_count(0), m_active_tick_count(0),
	m_min_duration_ns(0), m_max_duration_ns(UINT64_MAX), m_accumulated_duration_ns(m_min_duration_ns), m_max_ticks_per_cycle(1),
	m_time_scale(1ULL << 16ULL),
	m_event_callbacks(),
	m_shared(new shared{ 0, false }),
	m_enabled(true), m_kill(false), m_waiting(false), m_tick_lock(false)
{}

cc0::job::~job( void )
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

void cc0::job::cycle(uint64_t duration_ns)
{
	if (!m_tick_lock) {
		m_tick_lock = true;
		m_waiting = false;

		duration_ns = scale_time(duration_ns, m_time_scale);
		m_accumulated_duration_ns += duration_ns;

		const uint64_t min_dur_ns = m_min_duration_ns; // Save these so that child jobs can not affect, and break, the current ticking process.
		const uint64_t max_dur_ns = m_max_duration_ns;

		for (uint64_t i = m_max_ticks_per_cycle; i > 0; --i) {

			duration_ns = m_accumulated_duration_ns > max_dur_ns ? max_dur_ns : m_accumulated_duration_ns;

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

			if (duration_ns < min_dur_ns) {
				m_tick_lock = false;
				m_waiting = true;
				return;
			}
			m_accumulated_duration_ns -= duration_ns;

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
		}

		m_accumulated_duration_ns = max_dur_ns > 0 ? m_accumulated_duration_ns % max_dur_ns : 0;
		m_tick_lock = false;
	}
}

void cc0::job::kill( void )
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

void cc0::job::kill_children( void )
{
	for (cc0::job *c = m_child; c != nullptr; c = c->m_sibling) {
		c->kill();
	}
}

void cc0::job::sleep_for(uint64_t duration_ns)
{
	m_sleep_ns = m_sleep_ns > duration_ns ? m_sleep_ns : duration_ns;
}

void cc0::job::wake( void )
{
	m_sleep_ns = 0;
}

void cc0::job::ignore(const char *event)
{
	callback_tree *t = m_event_callbacks.get(0);
	if (t != nullptr) {
		t->remove(event);
	}
}

void cc0::job::ignore(const char *event, const cc0::job &sender)
{
	callback_tree *t = m_event_callbacks.get(sender.get_job_id());
	if (t != nullptr) {
		t->remove(event);
	}
}

void cc0::job::ignore(const cc0::job &sender)
{
	m_event_callbacks.remove(sender.get_job_id());
}

cc0::job *cc0::job::add_child(const char *type_name)
{
	cc0::job *p = nullptr;
	if (!is_killed()) {
		p = create_orphan(type_name);
		if (p != nullptr) {
			add_sibling(m_child, p);
			p->m_created_at_ns = get_local_time_ns();
			p->m_min_duration_ns = m_min_duration_ns;
			p->m_max_duration_ns = m_max_duration_ns;
			p->on_birth();
		}
	}
	return p;
}

void cc0::job::enable( void )
{
	m_enabled = true;
}

void cc0::job::disable( void )
{
	m_enabled = false;
}

bool cc0::job::is_killed( void ) const
{
	return m_kill;
}

bool cc0::job::is_alive( void ) const
{
	return !is_killed();
}

bool cc0::job::is_enabled( void ) const
{
	return !is_killed() && m_enabled;
}

bool cc0::job::is_disabled( void ) const
{
	return !is_enabled();
}

bool cc0::job::is_sleeping( void ) const
{
	return m_sleep_ns > 0;
}

bool cc0::job::is_awake( void ) const
{
	return !is_sleeping();
}

bool cc0::job::is_active( void ) const
{
	return is_enabled() && !is_sleeping();
}

bool cc0::job::is_inactive( void ) const
{
	return !is_active();
}

bool cc0::job::is_waiting( void ) const
{
	return m_waiting;
}

bool cc0::job::is_ready( void ) const
{
	return !is_waiting();
}

uint64_t cc0::job::get_job_id( void ) const
{
	return m_job_id;
}

void cc0::job::notify_parent(const char *event)
{
	if (m_parent != nullptr && is_active()) {
		notify(event, *m_parent);
	}
}

void cc0::job::notify_children(const char *event)
{
	for (cc0::job *c = m_child; c != nullptr && is_active(); c = c->m_sibling) {
		notify(event, *c);
	}
}

void cc0::job::notify_group(const char *event, cc0::job::query::results &group)
{
	query::result *r = group.get_results();
	while (r != nullptr && is_active()) {
		notify(event, *r->get_job().get_job());
		r = r->get_next();
	}
}

void cc0::job::notify(const char *event, cc0::job &target)
{
	target.get_notified(event, *this);
}

cc0::job::ref<> cc0::job::get_ref( void )
{
	return ref<job>(this);
}

uint64_t cc0::job::get_existed_for_ns( void ) const
{
	return m_existed_for_ns;
}

uint64_t cc0::job::get_active_for_ns( void ) const
{
	return m_active_for_ns;
}

uint64_t cc0::job::get_existed_tick_count( void ) const
{
	return m_existed_tick_count;
}

uint64_t cc0::job::get_active_tick_count( void ) const
{
	return m_active_tick_count;
}

cc0::job *cc0::job::get_parent( void )
{
	return m_parent;
}

const cc0::job *cc0::job::get_parent( void ) const
{
	return m_parent;
}

cc0::job *cc0::job::get_child( void )
{
	return m_child;
}

const cc0::job *cc0::job::get_child( void ) const
{
	return m_child;
}

cc0::job *cc0::job::get_sibling( void )
{
	return m_sibling;
}

const cc0::job *cc0::job::get_sibling( void ) const
{
	return m_sibling;
}

cc0::job *cc0::job::get_root( void )
{
	cc0::job *r = this;
	while (r->m_parent != nullptr) {
		r = r->m_parent;
	}
	return r;
}

const cc0::job *cc0::job::get_root( void ) const
{
	const cc0::job *r = this;
	while (r->m_parent != nullptr) {
		r = r->m_parent;
	}
	return r;
}

void cc0::job::set_local_time_scale(float time_scale)
{
	const uint64_t new_scale = uint64_t(time_scale * double(1ULL << 16ULL));
	m_time_scale = new_scale > 0 ? new_scale : 1;
	// TODO: Do we need to scale m_sleep here?
}

float cc0::job::get_local_time_scale( void ) const
{
	return float(m_time_scale / double(1ULL << 16ULL));
}

void cc0::job::set_global_time_scale(float time_scale)
{
	const uint64_t new_scale = (uint64_t(time_scale * double(1ULL << 16ULL)) << 16ULL) / get_parent_time_scale();
	m_time_scale = new_scale > 0 ? new_scale : 1;
}

float cc0::job::get_global_time_scale( void ) const
{
	return float(
		((m_time_scale * get_parent_time_scale()) >> 16ULL) / double(1ULL << 16ULL)
	);
}

uint64_t cc0::job::get_local_time_ns( void ) const
{
	return m_created_at_ns + m_existed_for_ns;
}

uint64_t cc0::job::get_created_at_ns( void ) const
{
	return m_created_at_ns;
}

cc0::job::query::results cc0::job::filter_children(const cc0::job::query &q)
{
	return get_children().filter_results(q);
}

cc0::job::query::results cc0::job::get_children( void )
{
	query::results r;
	job *c = get_child();
	while (c != nullptr) {
		r.add_result(*c);
		c = c->get_sibling();
	}
	return r;
}

uint64_t cc0::job::count_children( void ) const
{
	const job *n = get_child();
	uint64_t c = 0;
	while (n != nullptr) {
		++c;
		n = n->get_sibling();
	}
	return c;
}

uint64_t cc0::job::count_decendants( void ) const
{
	const job *n = get_child();
	uint64_t c = 0;
	while (n != nullptr) {
		c += (n->count_decendants() + 1);
		n = n->get_sibling();
	}
	return c;
}

void cc0::job::limit_tick_interval(uint64_t min_duration_ns, uint64_t max_duration_ns)
{
	m_min_duration_ns = min_duration_ns < max_duration_ns ? min_duration_ns : max_duration_ns;
	m_max_duration_ns = min_duration_ns > max_duration_ns ? min_duration_ns : max_duration_ns;
}

void cc0::job::unlimit_tick_interval( void )
{
	m_min_duration_ns = 0;
	m_max_duration_ns = UINT64_MAX;
}

void cc0::job::limit_tick_rate(uint64_t min_ticks_per_sec, uint64_t max_ticks_per_sec)
{
	limit_tick_interval(NS_PER_SEC / max_ticks_per_sec, NS_PER_SEC / min_ticks_per_sec);
}

void cc0::job::unlimit_tick_rate( void )
{
	unlimit_tick_interval();
}

uint64_t cc0::job::get_min_duration_ns( void ) const
{
	return m_min_duration_ns;
}

uint64_t cc0::job::get_max_duration_ns( void ) const
{
	return m_max_duration_ns;
}

uint64_t cc0::job::get_min_tick_per_sec( void ) const
{
	return m_max_duration_ns > 0 ? NS_PER_SEC / m_max_duration_ns : UINT64_MAX;
}

uint64_t cc0::job::get_max_tick_per_sec( void ) const
{
	return m_min_duration_ns > 0 ? NS_PER_SEC / m_min_duration_ns : UINT64_MAX;
}

bool cc0::job::is_tick_limited( void ) const
{
	return m_min_duration_ns != 0 || m_max_duration_ns != 0;
}

cc0::job *cc0::job::create_orphan(const char *type_name)
{
	cc0::job *j = nullptr;
	cc0::jobs_internal::instance_fn *i = m_products.get(type_name);
	if (i != nullptr) {
		cc0::jobs_internal::rtti *r = (*i)();
		if (r != nullptr) {
			j = r->cast<cc0::job>();
			if (j == nullptr) {
				delete r;
			}
			return j;
		}
	}
	return j;
}

bool cc0::job::has_enabled_children( void ) const
{
	const cc0::job *c = get_child();
	while (c != nullptr && c->is_disabled()) {
		c = c->get_sibling();
	}
	return c != nullptr;
}

uint64_t cc0::job::get_max_tick_per_cycle( void ) const
{
	return m_max_ticks_per_cycle;
}

void cc0::job::set_max_tick_per_cycle(uint64_t max_ticks_per_cyle)
{
	m_max_ticks_per_cycle = max_ticks_per_cyle > 0 ? max_ticks_per_cyle : 1;
}

void cc0::job::run(uint64_t fixed_duration_ns)
{
	on_birth();

	uint64_t duration_ns = fixed_duration_ns > get_min_duration_ns() ? fixed_duration_ns : get_min_duration_ns();

	while (is_enabled()) {
		
		const uint64_t start_ns = fixed_duration_ns == 0 ? std::chrono::high_resolution_clock::now().time_since_epoch().count() : 0;

		cycle(duration_ns);

		duration_ns = fixed_duration_ns == 0 ? std::chrono::high_resolution_clock::now().time_since_epoch().count() - start_ns : fixed_duration_ns;

		if (duration_ns < get_min_duration_ns()) {
			std::this_thread::sleep_for(std::chrono::nanoseconds(get_min_duration_ns() - duration_ns));
			duration_ns = get_min_duration_ns();
		}
	}
}

//
// defer
//

void cc0::jobs_internal::defer::on_tick(uint64_t)
{
	if (get_active_for_ns() >= m_target_time_ns) {
		notify_parent("defer");
		kill();
	}
}

cc0::jobs_internal::defer::defer( void ) :
	m_target_time_ns(get_active_for_ns())
{}

void cc0::jobs_internal::defer::set_delay(uint64_t ns)
{
	m_target_time_ns = get_active_for_ns() + ns;
}
