
namespace corryvreckan {

    template <typename T> void Clipboard::put(T* objects, const std::string& key) {
        // Do not insert empty sets:
        if(objects->empty()) {
            return;
        }

        // Iterator for data type:
        ClipboardData::iterator type = m_data.begin();

        /* If data type exists, returns iterator to offending key, if data type does not exist yet, creates new entry and
         * returns iterator to the newly created element
         */
        type = m_data.insert(type, ClipboardData::value_type(typeid(T), std::map<std::string, Objects*>()));

        // Insert data into data type element, silently fail if it exists already
        auto test = type->second.insert(std::make_pair(key, reinterpret_cast<Objects*>(objects)));
        if(!test.second) {
            LOG(WARNING) << "Not inserted for " << key;
        }
    }

    template <typename T> T* Clipboard::get(const std::string& key) const {
        if(m_data.count(typeid(T)) == 0 || m_data.at(typeid(T)).count(key) == 0) {
            return nullptr;
        }
        return reinterpret_cast<T*>(m_data.at(typeid(T)).at(key));
    }

} // namespace corryvreckan
