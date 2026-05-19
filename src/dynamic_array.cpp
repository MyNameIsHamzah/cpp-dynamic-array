#include <cstddef>
#include <iostream>
#include <string>

template <typename T>

class dynamicArray {
   private:
    std::size_t m_size{0};
    std::size_t m_capacity{0};
    T* m_data{nullptr};

   public:
    ~dynamicArray() { delete[] m_data; }

    void push_back(T x) {
        if (m_data == nullptr) {
            m_capacity += 1;
            m_data = new T[m_capacity];
        }

        if (m_size == m_capacity) {
            std::cout << "capacity reached!\n\n";
            m_capacity *= 2;
            T* tempPointer{new T[m_capacity]};

            for (std::size_t i{0}; i < m_size; ++i) {  // copy contents of old array to new one

                tempPointer[i] = m_data[i];
            }

            delete[] m_data;
            m_data = tempPointer;
            tempPointer = nullptr;
        }

        m_data[m_size] = x;
        m_size++;
    }

    std::size_t size() { return m_size; }
    std::size_t capacity() { return m_capacity; }
    T get(std::size_t index) { return m_data[index]; }
};

template <typename T>
void test(dynamicArray<T>& vec, T x) {
    vec.push_back(x);
    std::cout << "size: " << vec.size() << "\n";
    std::cout << "capacity: " << vec.capacity() << "\n";
    std::cout << "current contents: ";

    for (std::size_t i{0}; i < vec.size(); ++i) {
        std::cout << vec.get(i) << " ";
    }

    std::cout << "\n\n";
}

int main() {
    dynamicArray<int> myVec{};

    test(myVec, 1);
    test(myVec, 2);
    test(myVec, 3);
    test(myVec, 4);
    test(myVec, 5);

    return 0;
}