
#include <map>
#include <vector>
#include <tuple>


#ifndef APP__DIV_H
#define APP__DIV_H

struct cast
{
    using group = std::vector<int>;
    using part = std::tuple<group const*, int, int>;
    using range = std::vector<part>;
    static void test()
    {
        std::map<int, group> a{
            { 1,{ 1,2,3,4 } },
            { 2,{ 5,6,7 } },
            { 3,{ 8,9,10,11 } },
        };
        std::vector<range> out;
        map(a, out);
        for (auto& v : out)
        {
            printf("-------------\n");
            for (auto i : v)
            {
                printf("%p %d %d \n",std::get<0>(i),std::get<1>(i),std::get<2>(i));
            }
        }
    }
    static void map(std::map<int,group> const& a, std::vector<range>& out)
    {
        static const int size_unit = 3;
        int sized = 0; /// for size_unit
        range vec;
        for (auto& i : a)
        {
            int len = i.second.size();
            int j = 0; /// for vector<int>
            while (j<len)
            {
                int empty_len = len - j;
                if (sized + empty_len >= size_unit)
                {
                    int size_added = size_unit - sized;
                    vec.emplace_back(&i.second, j, size_added);
                    j += size_added;
                    sized += size_added;
                    out.push_back(vec);
                    vec.clear();
                    sized = 0;
                }
                else /// the i is used up
                {
                    int size_added = empty_len;
                    vec.emplace_back(&i.second, j, size_added);
                    j += size_added;
                    sized += size_added;
                }
            }
        }
        /// add the remnant part
        if (!vec.empty())
            out.push_back(vec);
    }
};


#endif