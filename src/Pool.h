#ifndef _POOL_H_
#define _POOL_H_

// FIXME: deque<T*> is not a good OO design.
//        Need to fix to deque<T>.

template <class T>
class Pool {
public:
    Pool(int unit_inc_sz, string name);
    ~Pool();

    void init();
    T* alloc();
    void reclaim(T* p_elm);

private:
    void increase();

private:
    deque<T*> m_pool;
    unsigned int m_unit_inc_sz;
    unsigned int m_num_inc;
    string m_name;
};

template <class T>
Pool<T>::Pool(int unit_inc_sz, string name)
{
    m_unit_inc_sz = unit_inc_sz;
    m_num_inc = 0;
    m_name = name;
}

template <class T>
Pool<T>::~Pool()
{
    m_pool.clear();
}

template <class T>
void Pool<T>::init()
{
    increase();
}

template <class T>
T* Pool<T>::alloc()
{
    if (m_pool.size() == 0)
        increase();

    T* p_elm = m_pool.front();
    m_pool.pop_front();
    p_elm->init();

    return p_elm;
}

template <class T>
void Pool<T>::reclaim(T* p_elm)
{
    p_elm->destroy();
    m_pool.push_back(p_elm);
}

template <class T>
void Pool<T>::increase()
{
    m_num_inc++;
    for (unsigned int i=0; i<m_unit_inc_sz; i++)
        m_pool.push_back( new T );

    // alert congestion
    if (m_num_inc > 0 && m_num_inc % 20 == 0)
        fprintf(stderr, "Congestion alert: Pool %s has %d elements at %.0lf clock\n", m_name.c_str(), m_unit_inc_sz*m_num_inc, simtime());
}

#endif // #ifndef _POOL_H_
