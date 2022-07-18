
///
/// \file removerepeating.h
///
/// \brief
///

#ifndef REMOVEREPEATING_H
#define REMOVEREPEATING_H


void removeRepeatingSlashBackSlash(char* str, const size_t length)
{
    size_t curIdx = 0;
    size_t lstIdx = 0;
    const size_t &endIdx = length;
    while (curIdx < endIdx) {
        if (curIdx > lstIdx) {
            str[lstIdx] = str[curIdx];
        }
        if ('/' == str[curIdx] || '\\' == str[curIdx]) {
            ++lstIdx;
            do {
                ++curIdx;
            } while (curIdx < endIdx && ('\\' == str[curIdx] || '/' == str[curIdx]));
        } else {
            ++lstIdx;
            ++curIdx;
        }
    }
    if (lstIdx < endIdx) {
        str[lstIdx] = '\0';
    }
}

inline void removeRepeatingSlashBackSlash(std::string& str)
{
    std::vector<char> vbuffer(str.size() + 1, '\0');
    vbuffer.insert(vbuffer.begin(), str.begin(), str.end());
    removeRepeatingSlashBackSlash(&vbuffer[0], vbuffer.size());
    str = vbuffer.data();
}


#endif // REMOVEREPEATING_H
