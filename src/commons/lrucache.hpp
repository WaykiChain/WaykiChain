// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2020 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <unordered_map>
#include <list>

/**
 * Least Recently Used Cache
 * the newest data is in the front
 */
template< class Key, class Data>
class CLruCache {
public:
    typedef std::pair<Key, Data> Item;
    typedef std::list< Item > Queue;
    typedef typename Queue::iterator QueueIterator;
    typedef std::unordered_map< Key, QueueIterator > Map;
    typedef std::function<uint32_t(const Item &item)> SizeFunc;
protected:
    Queue queue;
    Map index;
    uint32_t max_size = 0;
    uint32_t curr_size = 0;
    SizeFunc size_func = nullptr;

public:

    /** @brief Creates a cache that holds at most size worth of elements.
     *  @param maxSize maximum size of cache
     */
    CLruCache(const uint32_t maxSize, SizeFunc sizeFunc = nullptr) : max_size(maxSize), size_func(sizeFunc) {}

    /// Destructor - cleans up both index and storage
    ~CLruCache() { Clear(); }

    /** @brief Gets the current abstract size of the cache.
     *  @return current size
     */
    inline uint32_t GetSize() const { return queue.size(); }

    /** @brief Gets the maximum sbstract size of the cache.
     *  @return maximum size
     */
    inline uint32_t GetMaxSize() const { return max_size; }

    inline void SetMaxSize(uint32_t maxSize) {
        max_size = maxSize;
        CleanExcess();
    }

    const Queue& GetQueue() const {
        return queue;
    }

    /// Clears all storage and indices.
    void Clear() {
        queue.clear();
        index.clear();
    };

    /** @brief Checks for the existance of a key in the cache.
     *  @param key to check for
     *  @return bool indicating whether or not the key was found.
     */
    inline bool Exists( const Key &key ) const {
        return index.find( key ) != index.end();
    }

    /** @brief Removes a key-data pair from the cache.
     *  @param key to be removed
     */
    inline void Remove( const Key &key ) {
        auto mapIt = index.find( key );
        if (mapIt != index.end()) {
            queue.erase(mapIt->second);
            index.erase(mapIt);
        }
    }

    /** @brief Touches a key in the Cache and makes it the most recently used.
     *  @param key to be touched
     */
    inline Data*  Touch( const Key &key ) {
        return Get(key, true); // make touch
    }

    /** @brief get cached data.
     *  @param key to get data for
     *  @param touch_data whether or not to touch the data
     *  @return cached data pointer if existed, else nullptr
     */
    inline Data* Get( const Key &key, bool touch_data = true ) {
        auto mapIt = index.find( key );
        if( mapIt == index.end() )
            return nullptr;
        auto &qIt = mapIt->second;
        if( touch_data )
            TouchInList(qIt);
        return &(qIt->second);
    }

    /** @brief Inserts a key-data pair into the cache and removes entries if neccessary.
     *  @param key object key for insertion
     *  @param data object data for insertion
     *  @note This function checks key existance and touches the key if it already exists.
     */
    inline void Insert( const Key &key, const Data &data ) {

        auto mapIt = index.find(key);
        if(mapIt != index.end()) {
            // the key exists
            auto &qIt = mapIt->second;
            qIt->second = data;
            TouchInList(qIt);
        }

        // new cache item
        queue.emplace_front(key, data);
        auto qIt = queue.begin();
        index.emplace(key, qIt);
        curr_size += GetItemSize(*qIt);

        CleanExcess();
    }

private:
    inline void TouchInList(QueueIterator &qIt ) {
        // Move the found node to the head of the queue.
        if (qIt != queue.begin() && qIt != queue.end()) {
            queue.splice( queue.begin(), queue, qIt);
        }
    }

    inline void CleanExcess() {
        while( curr_size > max_size ) {
            // remove the last element.
            const auto &lastData = queue.back();
            curr_size -= GetItemSize(lastData);
            queue.pop_back();
            index.erase( lastData.first );
        }
    }

    inline uint32_t GetItemSize(const Item &item) {
        return size_func == nullptr ? 1 : size_func(item);
    }
};