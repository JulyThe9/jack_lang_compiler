#pragma once


template <typename T>
class ArenaAllocator
{
public:
    ArenaAllocator(size_t elems) : m_size(elems * sizeof(T)), m_occupied(0)
    {
        m_buffer = new char[m_size];
        m_offset = m_buffer;
    }

    ArenaAllocator(const ArenaAllocator &other) = delete;
    ArenaAllocator operator=(const ArenaAllocator &other) = delete;

    ~ArenaAllocator()
    {
        size_t i = 0;
        while (i < m_occupied)
        {        
            T *objPtr = reinterpret_cast<T*>(m_buffer + i);
            if (objPtr == NULL)
            {
                break;
            }
#ifdef MISC_DEBUG
            std::cout << (void*)(m_buffer + i) << '\n';
#endif

            objPtr->~T();
            i += sizeof(T);
        }
        delete[] m_buffer;
    }

    char *allocate()
    {
        char *offset = m_offset;
        m_offset += sizeof(T);
        m_occupied += sizeof(T);
        return offset;
    }

private:
    size_t m_size;
    size_t m_occupied;
    char *m_buffer;
    char *m_offset;
};