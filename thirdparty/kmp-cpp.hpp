#pragma once

// Santa Zhang - https://github.com/santazhang/kmp-cpp
// Public domain

#include <vector>
#include <iterator>

namespace kmp {

namespace detail {

// http://www.inf.fh-flensburg.de/lang/algorithmen/pattern/kmpen.htm
// requires random access iterator
// requires same value_type for iterators
// use partial template specializations to enforce those requirements
template <class NeedleIterator>
class pattern {
    NeedleIterator n_begin_;
    NeedleIterator n_end_;
    std::vector<long> skip_;

public:

    pattern(const NeedleIterator& begin,
            const NeedleIterator& end,
            std::random_access_iterator_tag): n_begin_(begin), n_end_(end) {
        // build KMP skip table
        long m = end - begin;
        if (m <= 0) {
            return;
        }
        long i = 0, j = -1;
        skip_.push_back(j);
        while (i < m) {
            while (j >= 0 && begin[i] != begin[j]) {
                j = skip_[j];
            }
            i++;
            j++;
            skip_.push_back(j);
        }
    }

    template <class HaystackIterator>
    long match_first(HaystackIterator h_begin,
                     HaystackIterator h_end,
                     std::random_access_iterator_tag,
                     std::true_type) const {

        long match = -1;
        long m = n_end_ - n_begin_;
        long n = h_end - h_begin;
        if (m == 0) {
            return 0;
        }
        if (n == 0) {
            return -1;
        }
        int i = 0, j = 0;
        while (i < n) {
            while (j >= 0 && h_begin[i] != n_begin_[j]) {
                j = skip_[j];
            }
            i++;
            j++;
            if (j == m) {
                match = i - j;
                break;
            }
        }
        return match;
    }

    template <class HaystackIterator>
    std::vector<long> match_all(HaystackIterator h_begin,
                                HaystackIterator h_end,
                                std::random_access_iterator_tag,
                                std::true_type,
                                bool overlap = true) const {

        std::vector<long> idx;
        long m = n_end_ - n_begin_;
        long n = h_end - h_begin;
        if (m == 0) {
            idx.push_back(0);
            return idx;
        }
        if (n == 0) {
            return idx;
        }
        int i = 0, j = 0;
        while (i < n) {
            while (j >= 0 && h_begin[i] != n_begin_[j]) {
                j = skip_[j];
            }
            i++;
            j++;
            if (j == m) {
                idx.push_back(i - j);
                if (overlap) {
                    j = skip_[j];
                } else {
                    j = 0;
                }
            }
        }
        return idx;
    }
};

} // namespace detail


template <class NeedleIterator>
class pattern {
    detail::pattern<NeedleIterator> pattern_;
public:

    pattern(const NeedleIterator& begin, const NeedleIterator& end)
        : pattern_(begin, end, typename std::iterator_traits<NeedleIterator>::iterator_category()) {}

    template <class HaystackIterator>
    long match_first(HaystackIterator h_begin, HaystackIterator h_end) {
        typedef typename std::iterator_traits<HaystackIterator>::iterator_category iterator_check;

        typedef typename std::is_same<
            typename std::iterator_traits<NeedleIterator>::value_type,
            typename std::iterator_traits<HaystackIterator>::value_type> type_check;

        return pattern_.match_first(h_begin, h_end, iterator_check(), type_check());
    }

    template <class HaystackIterator>
    std::vector<long> match_all(HaystackIterator h_begin, HaystackIterator h_end, bool overlap = true) {
        typedef typename std::iterator_traits<HaystackIterator>::iterator_category iterator_check;

        typedef typename std::is_same<
            typename std::iterator_traits<NeedleIterator>::value_type,
            typename std::iterator_traits<HaystackIterator>::value_type> type_check;

        return pattern_.match_all(h_begin, h_end, iterator_check(), type_check(), overlap);
    }
};

template <class NeedleIterator, class HaystackIterator>
long match_first(const NeedleIterator& n_begin,
                 const NeedleIterator& n_end,
                 const HaystackIterator& h_begin,
                 const HaystackIterator& h_end) {

    return pattern<NeedleIterator>(n_begin, n_end).match_first(h_begin, h_end);
}

template <class NeedleIterator, class HaystackIterator>
std::vector<long> match_all(const NeedleIterator& n_begin,
                            const NeedleIterator& n_end,
                            const HaystackIterator& h_begin,
                            const HaystackIterator& h_end,
                            bool overlap = true) {

    return pattern<NeedleIterator>(n_begin, n_end).match_all(h_begin, h_end, overlap);
}

} // namespace kmp
