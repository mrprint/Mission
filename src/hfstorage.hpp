#pragma once

#include <iterator>
#include <limits>
#include <assert.h>

#ifdef DEBUG
#include <cstring>
#endif

////////////////////////////////////////////////////////////////////////////////
// Хранилище гомогенных элементов с быстрой вставкой и удалением
////////////////////////////////////////////////////////////////////////////////

// Интерфейс
class HFStorageAstract
{
protected:
    size_t count;

public:
    HFStorageAstract() : count(0) {}
    virtual ~HFStorageAstract() {};
    virtual void* allocate() = 0;
    virtual void deallocate(void*) = 0;
    virtual size_t used_get() const = 0;
    virtual void* pointer_get(size_t i) const = 0;
    virtual size_t prev_get(size_t i) const = 0;
    virtual size_t next_get(size_t i) const = 0;
    size_t count_get() const { return count; }
};

// Хранилище с параметризованным типом индекса
template <class T>
class HFStorageParamI : public HFStorageAstract
{
    char *storage;
    size_t el_size, data_sz, amount;
    T used, free;

public:

    HFStorageParamI(size_t _el_size, size_t _amount)
    {
        assert(_el_size > 0 && _amount > 1);
        amount = _amount;
        el_size = block_size_get(_el_size + sizeof(T) * 2, 1);
        data_sz = el_size * amount;
        storage = new char[data_sz];
#ifdef DEBUG
        memset(storage, 0, data_sz);
#endif
        for (T i = 0; i < amount; ++i)
        {
            T *lnk = lnk_to(i);
            if (i == amount - 1)
            {
                lnk[0] = i - 1; lnk[1] = 0;
            }
            else if (i == 0)
            {
                lnk[0] = static_cast<T>(amount - 1); lnk[1] = 1;
            }
            else
            {
                lnk[0] = i - 1; lnk[1] = i + 1;
            }
        }
        used = std::numeric_limits<T>::max();
        free = 0;
    }

    virtual ~HFStorageParamI() { delete[] storage; }

    virtual void* allocate()
    {
        if (free == std::numeric_limits<T>::max())
            return 0;
        T ind = free;
        relink(ind, used, free);
        if (ind != std::numeric_limits<T>::max())
            ++count;
        return pointer_get(ind);
    }

    virtual void deallocate(void *ptr)
    {
        if (used == std::numeric_limits<T>::max())
            return;
        T ind = index_of(reinterpret_cast<char*>(ptr));
        assert(ind != std::numeric_limits<T>::max());
        if (ind != std::numeric_limits<T>::max())
        {
            --count;
            relink(ind, free, used);
#ifdef DEBUG
            memset(ptr_to(ind), 0, el_size - sizeof(T) * 2);
#endif
        }
    }

    virtual size_t used_get() const { return used; }

    virtual void* pointer_get(size_t i) const
    {
        assert(i < std::numeric_limits<T>::max());
        if (i >= std::numeric_limits<T>::max())
            return 0;
        return ptr_to(static_cast<T>(i));
    }

    virtual size_t prev_get(size_t i) const
    {
        if (i >= std::numeric_limits<T>::max())
            return std::numeric_limits<size_t>::max();
        T *lnk = lnk_to(static_cast<T>(i));
        return lnk[0];
    }

    virtual size_t next_get(size_t i) const
    {
        if (i >= std::numeric_limits<T>::max())
            return std::numeric_limits<size_t>::max();
        T *lnk = lnk_to(static_cast<T>(i));
        return lnk[1];
    }


private:

    T index_of(const char *const ptr) const
    {
        assert(ptr >= storage && ptr < storage + data_sz);
        return static_cast<T>((ptr - storage) / el_size);
    }

    char* ptr_to(T i) const
    {
        assert(i < amount);
        return storage + i * el_size;
    }

    T* lnk_to(T i) const
    {
        assert(i < amount);
        return reinterpret_cast<T*>(storage + (i + 1) * el_size - sizeof(T) * 2);
    }

    void connect(T a, T b)
    {
        T *la = lnk_to(a), *lb = lnk_to(b);
        la[1] = b; lb[0] = a;
    }

    T insert(T newi, T ptri)
    {
        if (ptri != std::numeric_limits<T>::max())
        {
            T *lnk = lnk_to(ptri);
            connect(lnk[0], newi);
            connect(newi, ptri);
            return ptri;
        }
        else
        {
            connect(newi, newi);
            return newi;
        }
    }

    void relink(T ind, T &_used, T &_free)
    {
        T *lnk = lnk_to(ind);
        // Не последний свободный
        if (lnk[1] != _free || lnk[0] != _free)
        {
            if (ind == _free)
            {
                // Исключаем текущий свободный
                _free = lnk[1];
            }
            connect(lnk[0], lnk[1]);
        }
        // Последний свободный
        else
        {
            _free = std::numeric_limits<T>::max();
        }
        // добавляем к занятым
        _used = insert(ind, _used);
    }

    static size_t block_size_get(size_t el_sz, T amount)
    {
        size_t raw_sz = amount * el_sz;
        return raw_sz + (raw_sz % sizeof(size_t) ? sizeof(size_t) : 0);
    }
};

// Базовое хранилище, адаптирующееся к объёму данных
class HFStorageBasic
{
    HFStorageAstract *allocator;

public:
    HFStorageBasic(size_t el_size, size_t amount)
    {
        if (amount - 1 > std::numeric_limits<unsigned>::max())
        {
            allocator = new HFStorageParamI<size_t>(el_size, amount);
        }
        else
            if (amount - 1 > std::numeric_limits<unsigned short>::max())
            {
                allocator = new HFStorageParamI<unsigned>(el_size, amount);
            }
            else
            {
                allocator = new HFStorageParamI<unsigned short>(el_size, amount);
            }
    }

    ~HFStorageBasic() { delete allocator; }
    void* allocate() { return allocator->allocate(); }
    void deallocate(void *ptr) { allocator->deallocate(ptr); }
    size_t used_get() const { return allocator->used_get(); }
    void* pointer_get(size_t i) const { return allocator->pointer_get(i); }
    size_t prev_get(size_t i) const { return allocator->prev_get(i); }
    size_t next_get(size_t i) const { return allocator->next_get(i); }
    size_t count_get() const { return allocator->count_get(); }
};

// Основной тип хранилища, параметризованный по типу хранящихся элементов
template <class T>
class HFStorage
{
public:
    typedef T value_type;

protected:
    HFStorageBasic storage;

public:
    class iterator : public std::iterator<std::forward_iterator_tag, T>
    {
        friend class HFStorage;

    private:
        HFStorage* ptr;
        size_t index;
        bool b_pass;

    public:
        iterator(HFStorage* p, bool is_end)
        {
            ptr = p;
            index = ptr->storage.used_get();
            b_pass = is_end;
        }

        iterator(HFStorage* p, size_t _index)
        {
            ptr = p;
            index = _index;
            b_pass = false;
        }

        iterator& operator++()
        {
            b_pass = true;
            index = ptr->storage.next_get(index);
            return *this;
        }

        T& operator*()
        {
            return *reinterpret_cast<T*>(ptr->storage.pointer_get(index));
        }

        T* operator->()
        {
            return reinterpret_cast<T*>(ptr->storage.pointer_get(index));
        }

        bool operator==(const iterator& other) const
        {
            return index == other.index && b_pass == other.b_pass;
        }

        bool operator!=(const iterator& other) const
        {
            return !(index == other.index && b_pass == other.b_pass);
        }
    };
    friend class iterator;

    HFStorage(size_t amount) : storage(sizeof(value_type), amount) {}
    HFStorage(size_t el_size, size_t amount) : storage(el_size, amount) {}

    void push_back(const value_type& val)
    {
        value_type *ptr = allocate();
        if (ptr)
            *ptr = val;
    }

    iterator erase(iterator position)
    {
        size_t next = storage.next_get(position.index);
        storage.deallocate(storage.pointer_get(position.index));
        iterator it(this, next);
        return it;
    }

    value_type* allocate()
    {
        return reinterpret_cast<value_type*>(storage.allocate());
    }

    void deallocate(value_type *ptr) { storage.deallocate(ptr); }

    size_t size() const { return storage.count_get(); }
    bool empty() const { return storage.count_get() == 0; };

    iterator begin()
    {
        iterator it(this, false);
        return it;
    }

    iterator end()
    {
        iterator it(this, true);
        return it;
    }

};