// Author: Patrick Wieschollek <mail@patwie.com>

#ifndef ENGINE_MISC_H
#define ENGINE_MISC_H

const int N = 19;

#define map2line(x,y) (((x) * 19 + (y)))
#define map3line(n,x,y) (( (n*19*19) +  (x) * 19 + (y)))
#define valid_pos(x) (((x>=0) && (x < N)))


typedef  std::pair<int, int> coord_t;

template<class T> inline
bool contains(const std::set<T>& container, const T& value)
{
    return container.find(value) != container.end();
}

#endif