/**
 * @file
 * @brief Template implementation of configuration
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

namespace corryvreckan {
    /**
     * @throws MissingKeyError If the requested key is not defined
     * @throws InvalidKeyError If the conversion to the requested type did not succeed
     * @throws InvalidKeyError If an overflow happened while converting the key
     */
    template <typename T> T Configuration::get(const std::string& key) const {
        try {
            auto node = parse_value(config_.at(key));
            used_keys_.markUsed(key);
            try {
                return corryvreckan::from_string<T>(node->value);
            } catch(std::invalid_argument& e) {
                throw InvalidKeyError(key, getName(), node->value, typeid(T), e.what());
            }
        } catch(std::out_of_range& e) {
            throw MissingKeyError(key, getName());
        } catch(std::invalid_argument& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(T), e.what());
        } catch(std::overflow_error& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(T), e.what());
        }
    }
    /**
     * @throws InvalidKeyError If the conversion to the requested type did not succeed
     * @throws InvalidKeyError If an overflow happened while converting the key
     */
    template <typename T> T Configuration::get(const std::string& key, const T& def) const {
        if(has(key)) {
            return get<T>(key);
        }
        return def;
    }

    /**
     * @throws MissingKeyError If the requested key is not defined
     * @throws InvalidKeyError If the conversion to the requested type did not succeed
     * @throws InvalidKeyError If an overflow happened while converting the key
     */
    template <typename T> std::vector<T> Configuration::getArray(const std::string& key) const {
        try {
            std::string str = config_.at(key);
            used_keys_.markUsed(key);

            std::vector<T> array;
            auto node = parse_value(str);
            for(auto& child : node->children) {
                try {
                    array.push_back(corryvreckan::from_string<T>(child->value));
                } catch(std::invalid_argument& e) {
                    throw InvalidKeyError(key, getName(), child->value, typeid(T), e.what());
                }
            }
            return array;
        } catch(std::out_of_range& e) {
            throw MissingKeyError(key, getName());
        } catch(std::invalid_argument& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(T), e.what());
        } catch(std::overflow_error& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(T), e.what());
        }
    }
    /**
     * @throws InvalidKeyError If the conversion to the requested type did not succeed
     * @throws InvalidKeyError If an overflow happened while converting the key
     */
    template <typename T> std::vector<T> Configuration::getArray(const std::string& key, const std::vector<T> def) const {
        if(has(key)) {
            return getArray<T>(key);
        }
        return def;
    }

    template <typename T1, typename T2> std::map<T1, T2> Configuration::getMap(const std::string& key) const {
        try {
            std::string str = config_.at(key);
            used_keys_.markUsed(key);

            std::map<T1, T2> map;
            auto node = parse_value(str);
            for(auto& child : node->children) {
                if(child->children.empty()) {
                    throw std::invalid_argument("map has no key-pair values");
                }

                // create pairs for final map
                std::pair<T1, T2> pair;
                int childId = 0;
                for(auto& subchild : child->children) {
                    // child Id == 0 : map key // child Id == 1 : map value
                    if(childId == 0) {
                        try {
                            pair.first = corryvreckan::from_string<T1>(subchild->value);
                        } catch(std::invalid_argument& e) {
                            throw InvalidKeyError(key, getName(), subchild->value, typeid(T1), e.what());
                        } catch(std::overflow_error& e) {
                            throw InvalidKeyError(key, getName(), subchild->value, typeid(T1), e.what());
                        }
                    } else if(childId == 1) {
                        try {
                            pair.second = corryvreckan::from_string<T2>(subchild->value);
                        } catch(std::invalid_argument& e) {
                            throw InvalidKeyError(key, getName(), subchild->value, typeid(T2), e.what());
                        } catch(std::overflow_error& e) {
                            throw InvalidKeyError(key, getName(), subchild->value, typeid(T2), e.what());
                        }
                    } else {
                        throw std::invalid_argument(
                            "map element id > 1 : map cannot have more than 2 elements [key, value]");
                    }
                    childId++;
                }
                map.insert(pair);
            }
            return map;
        } catch(std::out_of_range& e) {
            throw MissingKeyError(key, getName());
        } catch(std::invalid_argument& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(key), e.what());
        } catch(std::overflow_error& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(key), e.what());
        }
    }

    template <typename T1, typename T2>
    std::map<T1, T2> Configuration::getMap(const std::string& key, const std::map<T1, T2> def) const {
        if(has(key)) {
            return getMap<T1, T2>(key);
        }
        return def;
    }

    /**
     * @throws MissingKeyError If the requested key is not defined
     * @throws InvalidKeyError If the conversion to the requested type did not succeed
     * @throws InvalidKeyError If an overflow happened while converting the key
     */
    template <typename T> Matrix<T> Configuration::getMatrix(const std::string& key) const {
        try {
            std::string str = config_.at(key);
            used_keys_.markUsed(key);

            Matrix<T> matrix;
            auto node = parse_value(str);
            for(auto& child : node->children) {
                if(child->children.empty()) {
                    throw std::invalid_argument("matrix has less than two dimensions, enclosing brackets might be missing");
                }

                std::vector<T> array;
                // Create subarray of matrix
                for(auto& subchild : child->children) {
                    try {
                        array.push_back(corryvreckan::from_string<T>(subchild->value));
                    } catch(std::invalid_argument& e) {
                        throw InvalidKeyError(key, getName(), subchild->value, typeid(T), e.what());
                    }
                }
                matrix.push_back(array);
            }
            return matrix;
        } catch(std::out_of_range& e) {
            throw MissingKeyError(key, getName());
        } catch(std::invalid_argument& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(T), e.what());
        } catch(std::overflow_error& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(T), e.what());
        }
    }

    /**
     * @throws InvalidKeyError If the conversion to the requested type did not succeed
     * @throws InvalidKeyError If an overflow happened while converting the key
     */
    template <typename T> Matrix<T> Configuration::getMatrix(const std::string& key, const Matrix<T> def) const {
        if(has(key)) {
            return getMatrix<T>(key);
        }
        return def;
    }

    template <typename T> void Configuration::set(const std::string& key, const T& val, bool mark_used) {
        config_[key] = corryvreckan::to_string(val);
        used_keys_.registerMarker(key);
        if(mark_used) {
            used_keys_.markUsed(key);
        }
    }

    template <typename T>
    void Configuration::set(const std::string& key, const T& val, std::initializer_list<std::string> units) {
        auto split = corryvreckan::split<Units::UnitType>(corryvreckan::to_string(val));

        std::string ret_str;
        for(auto& element : split) {
            ret_str += Units::display(element, units);
            ret_str += ",";
        }
        ret_str.pop_back();
        config_[key] = ret_str;
        used_keys_.registerMarker(key);
    }

    template <typename T> void Configuration::setArray(const std::string& key, const std::vector<T>& val, bool mark_used) {
        // NOTE: not the most elegant way to support arrays
        std::string str;
        for(T el : val) { // NOLINT
            str += corryvreckan::to_string(el);
            str += ",";
        }
        str.pop_back();
        config_[key] = str;
        used_keys_.registerMarker(key);
        if(mark_used) {
            used_keys_.markUsed(key);
        }
    }

    template <typename T1, typename T2>
    void Configuration::setMap(const std::string& key, const std::map<T1, T2>& val, bool mark_used) {
        if(val.empty()) {
            return;
        }

        std::string str;
        str += "[";
        for(auto const& el : val) {
            str += "[" + corryvreckan::to_string(el.first) + "," + corryvreckan::to_string(el.second) + "],";
        }
        str.pop_back();
        str += "]";
        config_[key] = str;
        used_keys_.registerMarker(key);
        if(mark_used) {
            used_keys_.markUsed(key);
        }
    }

    template <typename T> void Configuration::setMatrix(const std::string& key, const Matrix<T>& val, bool mark_used) {
        // NOTE: not the most elegant way to support arrays
        if(val.empty()) {
            return;
        }

        std::string str = "[";
        for(auto& col : val) {
            str += "[";
            for(auto& el : col) {
                str += corryvreckan::to_string(el);
                str += ",";
            }
            str.pop_back();
            str += "],";
        }
        str.pop_back();
        str += "]";
        config_[key] = str;
        used_keys_.registerMarker(key);
        if(mark_used) {
            used_keys_.markUsed(key);
        }
    }

    template <typename T> void Configuration::setDefault(const std::string& key, const T& val) {
        if(!has(key)) {
            set<T>(key, val, true);
        }
    }

    template <typename T> void Configuration::setDefaultArray(const std::string& key, const std::vector<T>& val) {
        if(!has(key)) {
            setArray<T>(key, val, true);
        }
    }

    template <typename T1, typename T2>
    void Configuration::setDefaultMap(const std::string& key, const std::map<T1, T2>& val) {
        if(!has(key)) {
            setMap<T1, T2>(key, val, true);
        }
    }

    template <typename T> void Configuration::setDefaultMatrix(const std::string& key, const Matrix<T>& val) {
        if(!has(key)) {
            setMatrix<T>(key, val, true);
        }
    }
} // namespace corryvreckan
