#include <vector>

template<typename T>
int vectContains(const std::vector<T> &vect, const T &val) 
{
    for (size_t i = 0; i < vect.size(); ++i) 
    {
        if (vect[i] == val) 
        {
            return i; // Return the position if found
        }
    }
    return -1; // Return -1 if not found
}