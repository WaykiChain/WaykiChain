// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MINER_PBFTLIMITMAP_H
#define MINER_PBFTLIMITMAP_H

#include <map>
#include <vector>


/** STL-like map container that only keeps the N elements with the highest value. */
template <typename K, typename V>
class CPbftLimitmap {
public:
    typedef K key_type;
    typedef V mapped_type;
    typedef std::pair<const key_type, mapped_type> value_type;
    typedef typename std::map<K, V>::const_iterator const_iterator;
    typedef typename std::map<K, V>::size_type size_type;

private:
    std::map<K, V> map;
    typedef typename std::map<K, V>::iterator iterator;
    vector<K> vKeys;
    size_type nMaxSize;

public:
    explicit CPbftLimitmap(size_type nMaxSizeIn) {
        nMaxSize = nMaxSizeIn;
    }

    CPbftLimitmap(){ }

    V& operator[](const K& key){return map[key];}
    const_iterator begin() const { return map.begin(); }
    const_iterator end() const { return map.end(); }
    size_type size() const { return map.size(); }
    bool empty() const { return map.empty(); }
    const_iterator find(const key_type& k) const { return map.find(k); }
    size_type count(const key_type& k) const { return map.count(k); }

    void insert(const value_type& x) {

        std::pair<iterator, bool> ret = map.insert(x);
        if(ret.second && std::find(vKeys.begin(),vKeys.end(),x.first) == vKeys.end())
            vKeys.push_back(x.first);

        if (map.size() > nMaxSize) {
            eraseFirst();
        }

    }

    void eraseFirst() {
        K k = vKeys[0];
        vKeys.erase(vKeys.begin());
        map.erase(k);
    }


    void update(const_iterator itIn, const mapped_type& v) {

        iterator itTarget = map.erase(itIn, itIn);
        if (itTarget == map.end()) return;
        itTarget->second = v;

    }
    size_type max_size() const { return nMaxSize; }

    size_type max_size(size_type s) {
        assert(s > 0);
        while (map.size() > s) {
           eraseFirst();
        }
        nMaxSize = s;
        return nMaxSize;
    }

};



#endif //MINER_PBFTLIMITMAP_H
