#pragma once
#include <vector>
#include <memory>
#include <functional>

template <typename T>
class ObjectPool {
public:
    // 获取一个对象
    // Args... 是传递给对象 reset() 方法的参数
    template <typename... Args>
    T* acquire(Args&&... args) {
        if (m_pool.empty()) {
            // 如果池空了，创建一个新的 (完美转发构造参数，这里假设构造函数参数和reset一致，或者我们需要适配)
            // 为了简单，我们这里假设第一次创建时也调用构造函数，
            // 但通常对象池里的对象构造和 reset 是分开的。
            // 这里我们策略是：如果池空，new 一个新的，直接返回（构造函数负责初始化）
            // 注意：这意味着 T 的构造函数参数必须匹配 acquire 的参数，
            // 或者我们需要一个专门的 create 逻辑。
            // 鉴于 Projectile 的构造函数和 reset 参数很像，我们直接 new
            return new T(std::forward<Args>(args)...);
        } else {
            // 从池中取出一个
            T* obj = m_pool.back();
            m_pool.pop_back();
            // 重置状态 (复用关键)
            obj->reset(std::forward<Args>(args)...);
            return obj;
        }
    }

    // 归还对象
    void release(T* obj) {
        if (obj) {
            m_pool.push_back(obj);
        }
    }

    // 析构时清理所有缓存的对象
    ~ObjectPool() {
        for (auto obj : m_pool) {
            delete obj;
        }
        m_pool.clear();
    }

private:
    std::vector<T*> m_pool;
};