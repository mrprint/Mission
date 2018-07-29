#pragma once

#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <cstdint>
#include <assert.h>

#ifdef DEBUG
#include <cstring>
#endif

////////////////////////////////////////////////////////////////////////////////
// Хранилище гомогенных элементов с быстрыми вставкой и удалением
////////////////////////////////////////////////////////////////////////////////

namespace tool
{

    // Интерфейс
    class IHFStorage
    {
    protected:
        std::size_t count;

    public:
        IHFStorage() : count(0) {}
        virtual ~IHFStorage() {};
        virtual void* allocate() = 0;
        virtual void deallocate(void*) = 0;
        virtual std::size_t used_get() const = 0;
        virtual void* pointer_get(std::size_t i) const = 0;
        virtual std::size_t index_get(const void *const ptr) const = 0;
        virtual std::size_t prev_get(std::size_t i) const = 0;
        virtual std::size_t next_get(std::size_t i) const = 0;
        virtual bool full() const = 0;
        std::size_t count_get() const { return count; }
    };

    // Хранилище с параметризованным типом индекса
    template <typename T>
    class HFStorageParamI : public IHFStorage
    {
        std::unique_ptr<std::uint8_t[]> storage;
        std::size_t el_size, data_sz, amount;
        T used, free;

    public:

        HFStorageParamI(std::size_t _el_size, std::size_t _amount)
        {
            assert(_el_size > 0 && _amount > 1);
            amount = _amount;
            el_size = block_size_get(_el_size + sizeof(T) * 2, 1);
            data_sz = el_size * amount;
            storage = std::make_unique<std::uint8_t[]>(data_sz);
#ifdef DEBUG
            memset(storage.get(), 0, data_sz);
#endif
            for (T i = 0; i < amount; ++i) {
                T *lnk = lnk_to(i);
                if (i == amount - 1) {
                    lnk[0] = i - 1; lnk[1] = 0;
                } else if (i == 0) {
                    lnk[0] = static_cast<T>(amount - 1); lnk[1] = 1;
                } else {
                    lnk[0] = i - 1; lnk[1] = i + 1;
                }
            }
            used = std::numeric_limits<T>::max();
            free = 0;
        }

        virtual void* allocate() override
        {
            if (free == std::numeric_limits<T>::max())
                throw std::bad_alloc();
            T ind = free;
            relink(ind, used, free);
            ++count;
            return pointer_get(ind);
        }

        virtual void deallocate(void *ptr) override
        {
            if (used == std::numeric_limits<T>::max())
                return;
            T ind = index_of(reinterpret_cast<std::uint8_t*>(ptr));
            assert(ind != std::numeric_limits<T>::max());
            if (ind != std::numeric_limits<T>::max()) {
                --count;
                relink(ind, free, used);
#ifdef DEBUG
                memset(ptr_to(ind), 0, el_size - sizeof(T) * 2);
#endif
            }
        }

        virtual std::size_t used_get() const override { return used; }

        virtual void* pointer_get(std::size_t i) const override
        {
            assert(i < std::numeric_limits<T>::max());
            if (i >= std::numeric_limits<T>::max())
                return 0;
            return ptr_to(static_cast<T>(i));
        }

        virtual std::size_t index_get(const void *const ptr) const override
        {
            return static_cast<std::size_t>(index_of(reinterpret_cast<const std::uint8_t *const>(ptr)));
        }

        virtual std::size_t prev_get(std::size_t i) const override
        {
            if (i >= std::numeric_limits<T>::max())
                return std::numeric_limits<std::size_t>::max();
            T *lnk = lnk_to(static_cast<T>(i));
            return lnk[0];
        }

        virtual std::size_t next_get(std::size_t i) const override
        {
            if (i >= std::numeric_limits<T>::max())
                return std::numeric_limits<std::size_t>::max();
            T *lnk = lnk_to(static_cast<T>(i));
            return lnk[1];
        }

        virtual bool full() const override
        {
            return count == amount;
        }


    private:

        T index_of(const std::uint8_t *const ptr) const
        {
            assert(ptr >= storage.get() && ptr < storage.get() + data_sz);
            return static_cast<T>((ptr - storage.get()) / el_size);
        }

        std::uint8_t* ptr_to(T i) const
        {
            assert(i < amount);
            return storage.get() + i * el_size;
        }

        T* lnk_to(T i) const
        {
            assert(i < amount);
            return reinterpret_cast<T*>(storage.get() + (i + 1) * el_size - sizeof(T) * 2);
        }

        void connect(T a, T b)
        {
            T *la = lnk_to(a), *lb = lnk_to(b);
            la[1] = b; lb[0] = a;
        }

        T insert(T newi, T ptri)
        {
            if (ptri != std::numeric_limits<T>::max()) {
                T *lnk = lnk_to(ptri);
                connect(lnk[0], newi);
                connect(newi, ptri);
                return ptri;
            } else {
                connect(newi, newi);
                return newi;
            }
        }

        void relink(T ind, T &_used, T &_free)
        {
            T *lnk = lnk_to(ind);
            // Не последний свободный
            if (lnk[1] != _free || lnk[0] != _free) {
                if (ind == _free) {
                    // Исключаем текущий свободный
                    _free = lnk[1];
                }
                connect(lnk[0], lnk[1]);
            }
            // Последний свободный
            else {
                _free = std::numeric_limits<T>::max();
            }
            // добавляем к занятым
            _used = insert(ind, _used);
        }

        static std::size_t block_size_get(std::size_t el_sz, T amount)
        {
            std::size_t raw_sz = amount * el_sz;
            return raw_sz + ((raw_sz % sizeof(std::size_t)) ? sizeof(std::size_t) : 0);
        }
    };

    // Базовое хранилище, адаптирующееся к объёму данных
    class HFStorageBasic
    {
        std::unique_ptr<IHFStorage> allocator;

    public:
        HFStorageBasic(std::size_t el_size, std::size_t amount)
        {
            if (amount - 1 > std::numeric_limits<unsigned>::max()) {
                allocator = std::make_unique<HFStorageParamI<std::size_t>>(el_size, amount);
            } else
                if (amount - 1 > std::numeric_limits<unsigned short>::max()) {
                    allocator = std::make_unique<HFStorageParamI<unsigned>>(el_size, amount);
                } else {
                    allocator = std::make_unique<HFStorageParamI<unsigned short>>(el_size, amount);
                }
        }

        void* allocate() { return allocator->allocate(); }
        void deallocate(void *ptr) { allocator->deallocate(ptr); }
        std::size_t used_get() const { return allocator->used_get(); }
        void* pointer_get(std::size_t i) const { return allocator->pointer_get(i); }
        std::size_t index_get(const void *const ptr) const { return allocator->index_get(ptr); }
        std::size_t prev_get(std::size_t i) const { return allocator->prev_get(i); }
        std::size_t next_get(std::size_t i) const { return allocator->next_get(i); }
        std::size_t count_get() const { return allocator->count_get(); }
        bool full() const { return allocator->full(); }
    };

    // Основной тип хранилища, параметризованный по типу хранящихся элементов
    template <typename T>
    class HFStorage
    {
    public:
        using value_type = T;

    protected:
        HFStorageBasic storage;

    public:
        class iterator : public std::iterator<std::forward_iterator_tag, T>
        {
            friend class HFStorage;

        private:
            HFStorage * ptr;
            std::size_t index;
            bool b_pass;

        public:
            iterator(HFStorage* p, bool is_end)
            {
                ptr = p;
                index = ptr->storage.used_get();
                b_pass = is_end;
            }

            iterator(HFStorage* p, std::size_t _index)
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

        explicit HFStorage(std::size_t amount) : storage(sizeof(value_type), amount) {}
        HFStorage(std::size_t el_size, std::size_t amount) : storage(el_size, amount) {}
        ~HFStorage()
        {
            for (auto &i : *this) {
                i.~value_type();
            }
        }

        void push_back(const value_type& val)
        {
            value_type *ptr = allocate();
            if (ptr)
                *ptr = val;
        }

        iterator erase(iterator position)
        {
            std::size_t next = storage.next_get(position.index);
            storage.deallocate(storage.pointer_get(position.index));
            if (size() == 0) {
                return end();
            }
            return iterator(this, next);

        }

        value_type* allocate()
        {
#ifdef DEBUG
            assert(!groups->full());
#endif
            auto ptr = reinterpret_cast<value_type*>(storage.allocate());
            new (ptr) value_type();
            return ptr;
        }

        std::size_t allocate_idx()
        {
            auto ptr = allocate();
            return index_get(ptr);
        }

        void deallocate(value_type *ptr)
        {
            ptr->~value_type();
            storage.deallocate(ptr);
        }

        void deallocate_idx(std::size_t i)
        {
            deallocate(pointer_get(i));
        }

        std::size_t size() const { return storage.count_get(); }
        bool empty() const { return storage.count_get() == 0; }
        bool full() const { return storage.full(); }

        value_type* pointer_get(std::size_t i) const { return reinterpret_cast<value_type*>(storage.pointer_get(i)); }
        std::size_t index_get(const value_type *const ptr) const { return storage.index_get(ptr); }

        value_type& operator[](std::size_t i)
        {
            return *pointer_get(i);
        }

        iterator begin()
        {
            if (size() > 0) {
                iterator it(this, false);
                return it;
            } else {
                return end();
            }
        }

        iterator end()
        {
            iterator it(this, true);
            return it;
        }

    };

    ////////////////////////////////////////////////////////////////////////////////
    // Copyright(c) 2017 https://github.com/mrprint
    //
    // Permission is hereby granted, free of charge, to any person obtaining a copy 
    // of this software and associated documentation files(the "Software"), to deal 
    // in the Software without restriction, including without limitation the rights 
    // to use, copy, modify, merge, publish, distribute, sublicense, and / or sell 
    // copies of the Software, and to permit persons to whom the Software is 
    // furnished to do so, subject to the following conditions :
    //
    // The above copyright notice and this permission notice shall be included in 
    // all copies or substantial portions of the Software.
    //
    // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
    // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
    // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE 
    // AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
    // LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
    // OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
    // SOFTWARE.

}
