#pragma once
//插入gap
#include <cstddef>
#include <iterator>

namespace utils
{
    struct Insertion2   //插入
    {
        size_t index; //索引位置
        size_t n_num; //从该位置开始有number个n
        size_t gap_num; //从该位置开始有number个gap
    };
    struct Insertion   //插入
    {
        size_t index; //索引位置
        size_t number; //从该位置开始有number个gap

        bool operator==(const Insertion &rhs) const noexcept;

        template<typename InIt1, typename InIt2, typename OutIt>
        static void plus(InIt1 lhs_first, InIt1 lhs_last, InIt2 rhs_first, InIt2 rhs_last, OutIt dest) //
        {
            for (; lhs_first != lhs_last && rhs_first != rhs_last; )
                if (lhs_first->index < rhs_first->index)
                {
                    *dest++ = *lhs_first++;
                }
                else if (lhs_first->index > rhs_first->index)
                {
                    *dest++ = *rhs_first++;
                }
                else // lhs_first->index == rhs_first->index
                {
                    *dest++ = utils::Insertion({ lhs_first->index, lhs_first->number + rhs_first->number });
                    ++lhs_first;
                    ++rhs_first;
                }

            std::copy(lhs_first, lhs_last, dest);
            std::copy(rhs_first, rhs_last, dest);
        }

        // assume lhs >= rhs
        template<typename InIt1, typename InIt2, typename OutIt>
        static void minus(InIt1 lhs_first, InIt1 lhs_last, InIt2 rhs_first, InIt2 rhs_last, OutIt dest)
        {
            for (; rhs_first != rhs_last; ++lhs_first, ++rhs_first)
            {
                while (lhs_first->index != rhs_first->index)
                    *dest++ = *lhs_first++;

                const size_t difference = lhs_first->number - rhs_first->number;
                if (difference) *dest++ = utils::Insertion({ lhs_first->index, difference });
            }

            std::copy(lhs_first, lhs_last, dest);
        }

        template<typename InIt1, typename InIt2, typename OutIt>
        static void insert_gaps(InIt1 sequence_first, InIt1 sequence_last,
                InIt2 insertion_first, InIt2 insertion_last, OutIt dest, typename std::iterator_traits<InIt1>::value_type gap_symbol)
        {
            for (unsigned last_index = 0; insertion_first != insertion_last; ++insertion_first)
            {
                auto sequence_stop = sequence_first;
                std::advance(sequence_stop, insertion_first->index - last_index);

                dest = std::copy(sequence_first, sequence_stop, dest);
                dest = std::fill_n(dest, insertion_first->number, gap_symbol);

                sequence_first = sequence_stop;
                last_index = insertion_first->index;
            }

            std::copy(sequence_first, sequence_last, dest);
        }

    };
    
}



