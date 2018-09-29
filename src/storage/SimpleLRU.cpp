#include "SimpleLRU.h"

namespace Afina {
    namespace Backend {

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Put(const std::string &key, const std::string &value) {
            lru_node *node = new lru_node(key, value, _lru_end);
            //_lru_index[key] = *new std::reference_wrapper<lru_node>(*node);не работает хз почему
            auto res = _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(key),
                                                        std::reference_wrapper<lru_node>(*node)));
            if (!res.second) {
                std::size_t length = value.size();
                if (length > _max_size) {
                    return false;
                }
                _size -= res.first->second.get().value.size();
                _size += length;
                while (length + _size > _max_size) {
                    Delete(_lru_head->key);
                    _del_from_list(*_lru_head);
                }
                res.first->second.get().value = value;
                return true;
            }
            _add_to_list(key, value);
            return true;
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
            if (_lru_index.find(key) != _lru_index.end()) {
                return false;
            }
            lru_node *node = new lru_node(key, value, _lru_end);
            _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(key),
                                             std::reference_wrapper<lru_node>(*node)));
            return _add_to_list(key, value);
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Set(const std::string &key, const std::string &value) {
            auto it = _lru_index.find(key);
            if (it == _lru_index.end()) {
                return false;
            }
            lru_node &val = it->second.get();
            Delete(key);
            Put(value, val.value);
            return true;
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Delete(const std::string &key) {
            auto it = _lru_index.find(key);
            if (it == _lru_index.end()) {
                return false;
            }
            _lru_index.erase(it);
            _del_from_list(it->second.get());
            return true;
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Get(const std::string &key, std::string &value) const {
            auto it = _lru_index.find(key);
            if (it == _lru_index.end()) {
                return false;
            }
            return true;
            //return _add_to_list(key, value);
        }

        void SimpleLRU::_del_from_list(lru_node &node) {
            std::size_t length = node.key.size() + node.value.size();
            _size -= length;
            if (node.next != nullptr) {
                node.next->prev = node.prev;
            } else {
                _lru_end = node.prev;
            }
            if (node.prev != nullptr) {
                node.prev->next.swap(node.next);
            } else {
                _lru_head.swap(node.next);
            }
            node.next.reset();
        }


        bool SimpleLRU::_add_to_list(const std::string &key, const std::string &value) {
            std::size_t length = key.size() + value.size();
            if (length > _max_size) {
                return false;
            }
            while (length + _size > _max_size) {
                Delete(_lru_head->key);
                _del_from_list(*_lru_head);
            }
            _size += length;
            if (_lru_end != nullptr) {
                _lru_end->next.reset(new lru_node(key, value, _lru_end));
                _lru_end = _lru_end->next.get();
            } else {
                _lru_head.reset(new lru_node(key, value, _lru_end));
                _lru_end = _lru_head.get();
            }
            return true;
        }
    } // namespace Backend
} // namespace Afina
