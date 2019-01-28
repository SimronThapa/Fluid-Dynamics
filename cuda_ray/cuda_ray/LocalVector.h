template<typename T>
class LocalVector
{
private:
	T* m_begin;
	T* m_end;

	size_t capacity;
	size_t length;
	__device__ void expand() {
		capacity *= 2;
		size_t tempLength = (m_end - m_begin);
		T* tempBegin = new T[capacity];

		memcpy(tempBegin, m_begin, tempLength * sizeof(T));
		delete[] m_begin;
		m_begin = tempBegin;
		m_end = m_begin + tempLength;
		length = static_cast<size_t>(m_end - m_begin);
	}
public:
	__device__  explicit LocalVector() : length(0), capacity(16) {
		m_begin = new T[capacity];
		m_end = m_begin;
	}
	__device__ T& operator[] (unsigned int index) {
		return *(m_begin + index);//*(begin+index)
	}
	__device__ T* begin() {
		return m_begin;
	}
	__device__ T* end() {
		return m_end;
	}
	__device__ ~LocalVector()
	{
		delete[] m_begin;
		m_begin = nullptr;
	}

	__device__ void add(T t) {

		if ((m_end - m_begin) >= capacity) {
			expand();
		}

		new (m_end) T(t);
		m_end++;
		length++;
	}
	__device__ T pop() {
		T endElement = (*m_end);
		delete m_end;
		m_end--;
		return endElement;
	}

	__device__ size_t getSize() {
		return length;
	}
};