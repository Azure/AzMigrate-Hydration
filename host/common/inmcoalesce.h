#ifndef INMCOALESCE__H_
#define INMCOALESCE__H_

#include <cstddef>

template <class OFF_T, class LEN_T, class NUM_T>
 void InmCoalesce(const size_t &coalescelimit, OFF_T *offsets, LEN_T *lengths, LEN_T *maxcoveredindexes, NUM_T &n)
{
    OFF_T currentcover;
    OFF_T nextcover;
    OFF_T add;

    if (0 == n)
    {
        return;
    }

    for (NUM_T i = 0; i < n-1; i++)
    {
        for (NUM_T k = i+1; k < n; k++)
        {
            currentcover = offsets[i] + lengths[i];
            nextcover = offsets[k] + lengths[k];
            add = (nextcover > currentcover) ? (nextcover - currentcover) : 0;
            if ((offsets[k] >= offsets[i]) && (offsets[k] <= currentcover) && 
                (lengths[i]+add <= coalescelimit))
            {
                lengths[i] += add; 
                memmove(offsets + k, offsets + k + 1, ((n - (k + 1)) * sizeof(offsets[k])));
                memmove(lengths + k, lengths + k + 1, ((n - (k + 1)) * sizeof(lengths[k])));
                maxcoveredindexes[i]++;
                memmove(maxcoveredindexes + k, maxcoveredindexes + k + 1, ((n - (k + 1)) * sizeof(maxcoveredindexes[k])));
                n--;
                k--;
            }
            else
            {
                break;
            }
        }
    }
}

#endif /* INMCOALESCE__H_ */
